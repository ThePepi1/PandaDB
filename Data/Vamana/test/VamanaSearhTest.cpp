#include <gtest/gtest.h>
#include <hdf5.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "../../Data/VectorStream/includes/VectorStreams.h"
#include "../../Data/VectorStream/includes/NodeStream.h"
#include "../../Data/Vamana/includes/ClusterNode.h"
#include "../../Data/Space/includes/metadata.h"
#include "../includes/Vamana.h"
#include "../../KMeans/includes/trainer.h"
#include "../includes/Vector/FactoryVectorCPU.h"
#include "../../KMeans/includes/productquntizier.h"
#include "../../KMeans/includes/codeBook.h"
#include "../../Metrics/includes/MetricFactory.h"
#include "../../Metrics/includes/register.h"

namespace fs = std::filesystem;

// ============================ PUTANJE (env ima prednost) ============================
// env: VAMANA_LINSTORE, VAMANA_HDF5, VAMANA_INDEX
static const std::string kDir       = "/home/pepi/Documents/projects/PandaDB/downloadedDataTest/";
static const std::string kLinstore  = kDir + "sift-128-euclidean.linstore";
static const std::string kHdf5Path  = kDir + "sift-128-euclidean.hdf5";     // cita se samo test + neighbors
static const std::string kIndexBase = kDir + "sift-128-euclidean";   // isto kao indexBase u build testu
// ====================================================================================

namespace {

constexpr size_t K              = 10;
constexpr size_t MAX_QUERIES    = 1000;
constexpr double kPqTrainFrac   = 0.5;    // PQ se trenira nad delom train-a (samo za aproksimaciju u search-u)
constexpr int    kMaxIterations = 25;
constexpr double MIN_RECALL     = 0.80;   // prilagodi parametrima (Ef, beamSize, PQ)

std::string envOr(const char* name, const std::string& def) {
    const char* e = std::getenv(name);
    return e ? e : def;
}

template <typename T>
struct Matrix {
    std::vector<T> data;
    size_t rows = 0, cols = 0;
    const T* row(size_t i) const { return data.data() + i * cols; }
};

// cita 2D dataset iz hdf5; HDF5 sam konvertuje tip (npr. float32 u fajlu -> double u memoriji)
template <typename T>
bool readDataset(const std::string& path, const char* name, hid_t type,
                 size_t maxRows, Matrix<T>& out) {
    hid_t f = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (f < 0) return false;
    hid_t d = H5Dopen2(f, name, H5P_DEFAULT);
    if (d < 0) { H5Fclose(f); return false; }

    hid_t sp = H5Dget_space(d);
    hsize_t dims[2] = {0, 0};
    H5Sget_simple_extent_dims(sp, dims, nullptr);
    out.rows = maxRows ? std::min<size_t>(maxRows, dims[0]) : dims[0];
    out.cols = dims[1];
    out.data.resize(out.rows * out.cols);

    hsize_t start[2] = {0, 0}, count[2] = {out.rows, dims[1]};
    H5Sselect_hyperslab(sp, H5S_SELECT_SET, start, nullptr, count, nullptr);
    hid_t mem = H5Screate_simple(2, count, nullptr);
    herr_t st = H5Dread(d, type, mem, sp, H5P_DEFAULT, out.data.data());

    H5Sclose(mem); H5Sclose(sp); H5Dclose(d); H5Fclose(f);
    return st >= 0;
}

using VecPtr = decltype(std::declval<data::IVector&>().clone(0, 0));

} // namespace

class VamanaSearchTest : public ::testing::Test {
protected:
    // ------------------------------------------------------------------
    // Jednom za ceo suite: ucitaj upite, istreniraj PQ, izracunaj PQ kodove.
    // Indeks se NE gradi, samo se otvara postojeci fajl.
    // ------------------------------------------------------------------
    static void SetUpTestSuite() {
        metrics::register_builtin_metrics();

        const std::string linstore = envOr("VAMANA_LINSTORE", kLinstore);
        const std::string h5       = envOr("VAMANA_HDF5", kHdf5Path);
        indexBase                  = envOr("VAMANA_INDEX", kIndexBase);

        if (!fs::exists(linstore))              { skipReason = "nema linstore: " + linstore; return; }
        if (!fs::exists(h5))                    { skipReason = "nema hdf5: " + h5; return; }
        if (!fs::exists(indexBase + ".vamana")) { skipReason = "nema indeksa: " + indexBase + ".vamana (prvo pokreni build test)"; return; }

        if (!readDataset<double>(h5, "test", H5T_NATIVE_DOUBLE, MAX_QUERIES, queries) ||
            !readDataset<int>(h5, "neighbors", H5T_NATIVE_INT, MAX_QUERIES, gt)) {
            skipReason = "hdf5 nema 'test'/'neighbors'";
            return;
        }

        storage = std::make_unique<DataStructures::LinearVectorStorage>(
            linstore, DataStructures::OpenMode::Read, false);
        n = static_cast<size_t>(storage->getSize());
        if (n == 0) { skipReason = "linstore je prazan"; return; }

        factory = std::make_unique<data::FactoryVectorCPU>();
        metric  = metrics::MetricFactory::instance().create_metric("euclide");

        // ---- Metadata: MORA da se poklapa sa build testom ----
        metadata.NeighborsSize = 64;
        metadata.R             = 32;
        metadata.K             = 2;
        metadata.L             = 50;
        metadata.alpha         = 1.2;
        metadata.Ef            = 256;     // search parametri
        metadata.beamSize      = 4;
        // ---- PQ (isti parametri kao u build testu) ----
        {
            auto ts = storage->getVectorStream(kPqTrainFrac, *factory);
            dim = ts->getDim();
            metadata.vectorDim = dim;

            pq = std::make_unique<KMeans::ProductQuantizer>(8, 256, dim, *factory);
            KMeans::CodeBook codeBook(2, dim, *factory);   // treba samo da bi trainer radio
            KMeans::Trainer trainer;

            auto t0 = std::chrono::steady_clock::now();
            trainer.train(*pq, codeBook, *ts, *metric, kMaxIterations);
            std::cerr << "PQ train: "
                      << std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count()
                      << " s\n";
        }

        // ---- PQ kodovi svih n vektora, redom (id == pozicija) + probe vektori ----
        {
            const std::set<size_t> probeIds = {0, 1, n / 4, n / 2, n - 1};
            auto s = storage->getVectorStream(1.0, *factory);
            codebookAssignments.reserve(n);
            size_t id = 0;
            while (s->hasNext()) {
                auto vec = s->next();
                if (probeIds.count(id)) probes.emplace_back(id, vec->clone(0, vec->Dimension()));
                codebookAssignments.push_back(pq->quantize(*vec, *metric));   // <-- PRILAGODI ako encode vraca drugi tip
                ++id;
            }
            if (id != n) {
                skipReason = "stream vratio " + std::to_string(id) + " vektora, a size je " + std::to_string(n);
                return;
            }
        }
        std::cout << "PQ kodovi i probe vektori ucitani, n=" << n << " dim=" << dim << "\n";

        vamana = std::make_unique<DataStructures::Vamana>(indexBase);
        ready  = true;
    }

    static void TearDownTestSuite() {
        vamana.reset();
        probes.clear();
        codebookAssignments.clear();
        pq.reset();
        storage.reset();
    }

    void SetUp() override {
        if (!ready) GTEST_SKIP() << skipReason;
    }

    // vektor gleda u memoriju matrice `queries` (owner = false), pa matrica mora da ostane živa.
    // Ako je u tvom Vector_CPU značenje `owner` obrnuto, okreni na true.
    static std::unique_ptr<data::IVector> makeQuery(const double* v, size_t d) {
        return factory->createVector(static_cast<int>(d), const_cast<double*>(v), false);
    }

    static std::set<size_t> truth(size_t qi) {
        return std::set<size_t>(gt.row(qi), gt.row(qi) + K);
    }

    static double recallAt(size_t Q) {
        double hits = 0;
        for (size_t qi = 0; qi < Q; ++qi) {
            auto q = makeQuery(queries.row(qi), queries.cols);
            auto t = truth(qi);
            auto res = vamana->search(*q, *pq, codebookAssignments, *factory, K, metadata, *metric);
            for (auto& r : res) {
                if (t.count(static_cast<size_t>(r->getId()))) ++hits;

            }
        }
        return hits / (static_cast<double>(Q) * K);
    }

    static inline bool        ready = false;
    static inline std::string skipReason = "nije inicijalizovano";
    static inline std::string indexBase;
    static inline size_t      n = 0, dim = 0;

    static inline Matrix<double> queries;
    static inline Matrix<int>    gt;

    static inline std::unique_ptr<DataStructures::LinearVectorStorage> storage;
    static inline std::unique_ptr<data::FactoryVectorCPU>              factory;
    static inline decltype(metrics::MetricFactory::instance().create_metric("euclide")) metric;
    static inline std::unique_ptr<KMeans::ProductQuantizer>            pq;
    static inline DataStructures::Metadata                             metadata;
    static inline std::vector<std::vector<long long>>                  codebookAssignments;
    static inline std::vector<std::pair<size_t, VecPtr>>               probes;
    static inline std::unique_ptr<DataStructures::Vamana>              vamana;
};

TEST_F(VamanaSearchTest, SuiteReady) {
 {
    DataStructures::LinearNodeStorage probe(indexBase + ".vamana", DataStructures::OpenMode::Read, true);
    std::vector<bool> visited(storage->getSize(), false);
std::queue<size_t> q;

visited[0] = true;
q.push(0);

size_t reached = 1;

while(!q.empty()){
    size_t id = q.front();
    q.pop();

    auto nodes = probe.loadBeam(
        {static_cast<long long>(id)},
        factory.get(),
        metadata.NeighborsSize
    );

    if(nodes.empty())
        continue;

    for(auto neighbour : nodes[0]->getNeighbors()){
        if(neighbour == DataStructures::EMPTY_NEIGHBOR)
            continue;

        if(neighbour < 0 ||
           static_cast<size_t>(neighbour) >= probe.getSize())
            continue;

        size_t n = static_cast<size_t>(neighbour);

        if(!visited[n]){
            visited[n] = true;
            q.push(n);
            ++reached;
        }
    }
}

std::cerr << "reachable from 0: "
          << reached << " / "
          << probe.getSize() << "\n";
}
}
// ---------------- recall ----------------
TEST_F(VamanaSearchTest, RecallAtK) {
    auto t0 = std::chrono::steady_clock::now();
    const double r = recallAt(queries.rows);
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::cout << "recall@" << K << " = " << r << "  |  "
              << queries.rows / secs << " QPS (1 nit), "
              << secs / queries.rows * 1000.0 << " ms/query\n";
    EXPECT_GE(r, MIN_RECALL);
}

// recall treba da raste sa Ef; ako ne raste, bug je u grafu ili search-u
TEST_F(VamanaSearchTest, RecallGrowsWithEf) {
    const size_t Q = std::min<size_t>(200, queries.rows);
    const auto origEf = metadata.Ef;
    double prev = 0;
    for (size_t ef : {16u, 32u, 64u, 128u, 256u}) {
        metadata.Ef = ef;
        const double r = recallAt(Q);
        std::cout << "Ef=" << ef << "  recall@" << K << " = " << r << "\n";
        EXPECT_GE(r + 0.02, prev) << "recall pao kad je Ef poraslo na " << ef;
        prev = r;
    }
    metadata.Ef = origEf;
}

TEST_F(VamanaSearchTest, RecallVsBeamWidth) {
    const size_t Q = std::min<size_t>(200, queries.rows);
    const auto origBeam = metadata.beamSize;
    for (size_t b : {1u, 4u, 8u}) {
        metadata.beamSize = b;
        std::cout << "beam=" << b << "  recall@" << K << " = " << recallAt(Q) << "\n";
    }
    metadata.beamSize = origBeam;
}

// ---------------- struktura rezultata ----------------
TEST_F(VamanaSearchTest, ResultsSortedBoundedUnique) {
    for (size_t qi = 0; qi < 50; ++qi) {
        auto q = makeQuery(queries.row(qi), queries.cols);
        auto res = vamana->search(*q, *pq, codebookAssignments, *factory, K, metadata, *metric);
        ASSERT_LE(res.size(), K);
        std::set<size_t> ids;
        for (size_t i = 0; i < res.size(); ++i) {
            EXPECT_LT(static_cast<size_t>(res[i]->getId()), n);
            EXPECT_TRUE(ids.insert(static_cast<size_t>(res[i]->getId())).second)
                << "duplikat u rezultatu";
            if (i) EXPECT_LE((*metric)(res[i - 1]->getVector(), *q),
                             (*metric)(res[i]->getVector(), *q));
        }
    }
}


// ---------------- ivicni slucajevi ----------------
TEST_F(VamanaSearchTest, EdgeCases) {
    auto q = makeQuery(queries.row(0), queries.cols);

    EXPECT_TRUE(vamana->search(*q, *pq, codebookAssignments, *factory, 0, metadata, *metric).empty());

    auto big = vamana->search(*q, *pq, codebookAssignments, *factory, 5000, metadata, *metric);
    EXPECT_LE(big.size(), n);
    EXPECT_FALSE(big.empty());

    std::vector<std::vector<long long>> empty;
    EXPECT_TRUE(vamana->search(*q, *pq, empty, *factory, K, metadata, *metric).empty());
}

