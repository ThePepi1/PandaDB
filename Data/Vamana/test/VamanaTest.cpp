#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>

#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "../../Data/VectorStream/includes/VectorStreams.h"
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

namespace {
    const std::string kBenchmarkFilePath =
        "/home/pepi/Documents/projects/PandaDB/downloadedDataTest/sift-128-euclidean.linstore";
    constexpr double kStreamFraction = 0.2;   // lower it (e.g. 0.1) for quick runs
    constexpr int kMaxIterations = 1;

    template <typename F>
    long long timeMs(F&& f) {
        auto s = std::chrono::high_resolution_clock::now();
        f();
        auto e = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(e - s).count();
    }
}

class VamanaBuildPerfTest : public ::testing::Test {
protected:
    std::unique_ptr<DataStructures::LinearVectorStorage> storage;
    fs::path workDir;

    void SetUp() override {
        storage = std::make_unique<DataStructures::LinearVectorStorage>(kBenchmarkFilePath);
        ASSERT_GT(storage->getSize(), 0);
        metrics::register_builtin_metrics();

        // Dedicated dir: Vamana::removeDumpFiles() deletes EVERY *.dump in the parent folder.
        workDir = fs::temp_directory_path() / "pandadb_vamana_perf";
        fs::remove_all(workDir);
        fs::create_directories(workDir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(workDir, ec);
    }
};

TEST_F(VamanaBuildPerfTest, TrainQuantizeAndBuildIndex) {
    data::FactoryVectorCPU factory;
    auto metric = metrics::MetricFactory::instance().create_metric("euclide");

    // ---- 1. Train PQ + codebook (KMeans) ----
    auto stream = storage->getVectorStream(kStreamFraction, factory);
    const auto dim = stream->getDim();

    KMeans::ProductQuantizer pq(8, 256, dim, factory);
    KMeans::CodeBook codeBook(256, dim, factory);
    KMeans::Trainer trainer;

    auto trainMs = timeMs([&] { trainer.train(pq, codeBook, *stream, *metric, kMaxIterations); });
    std::cerr << "train: " << trainMs << " ms\n";

    // ---- 2. Metadata ---- (ADAPT: field names taken from Vamana.cpp, types/values are guesses)
    DataStructures::Metadata metadata;
    metadata.vectorDim     = dim;
    metadata.NeighborsSize = 64;    // max out-degree stored per node
    metadata.R             = 32;    // initial random neighbours
    metadata.K             = 2;     // clusters each vector is assigned to
    metadata.L             = 100;   // build search list size
    metadata.alpha         = 1.2;
    metadata.Ef            = 100;
    metadata.beamSize      = 4;

    // ---- 3. Raw vectors -> LinearStorage<Node> ----
    const std::string nodesPath = (workDir / "nodes.storage").string();
    DataStructures::LinearStorage<DataStructures::Node> nodeStorage(
        nodesPath, metadata.vectorDim, 0, metadata.NeighborsSize);

    long long count = 0;
    auto convertMs = timeMs([&] {
        auto src = storage->getVectorStream(kStreamFraction, factory);
        while (src->hasNext()) {
            auto vec = src->next();                       // ADAPT: whatever your stream returns
            DataStructures::Node node = DataStructures::Node(count, vec->clone(0, vec->Dimension()),metadata.NeighborsSize); // ADAPT: Node constructor
            nodeStorage.saveVector(node);
            ++count;
        }
    });
    std::cerr << "converted " << count << " vectors to nodes: " << convertMs << " ms\n";
    ASSERT_GT(count, 0);

    // ---- 4. Build Vamana ----
    const std::string indexBase = (workDir / "index").string();
    DataStructures::Vamana vamana(indexBase);
    std::cerr << "Vamana index will be written to: " << indexBase << ".vamana\n";

    bool ok = false;
    auto buildMs = timeMs([&] {
        ok = vamana.buildIndex(nodeStorage, codeBook, factory, metadata, *metric);
    });
    std::cerr << "buildIndex: " << buildMs << " ms ("
              << buildMs / 1000.0 << " s)\n";

    ASSERT_TRUE(ok);
    EXPECT_TRUE(fs::exists(indexBase + ".vamana"));
    EXPECT_GT(fs::file_size(indexBase + ".vamana"), 0u);
}