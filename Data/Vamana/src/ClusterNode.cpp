#include "../includes/ClusterNode.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <tbb/blocked_range.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

namespace DataStructures {


    namespace {

        struct DenseCand {
            double   dist;
            uint32_t id;              
            bool     expanded = false;
        };

        inline bool byDist(const DenseCand& a, const DenseCand& b) { return a.dist < b.dist; }

        struct Scratch {
            explicit Scratch(std::size_t n) : mark(n, 0) {}

            void newEpoch() {
                if (++epoch == 0) { std::fill(mark.begin(), mark.end(), 0u); epoch = 1; }
            }

            std::vector<uint32_t>  mark;     
            uint32_t               epoch = 0;
            std::vector<DenseCand> list;     
            std::vector<DenseCand> visited;  
            std::vector<uint32_t>  out;     
        };

    }

    ClusterGraph ClusterGraph::load(LinearStorage<Node>& storage,
                                    data::FactoryVector& factory,
                                    std::size_t slots)
    {
        const std::size_t n = static_cast<std::size_t>(storage.getSize());

        ClusterGraph g(slots);
        g.ids_.reserve(n);
        g.nodes_.reserve(n * 2);

        for (std::size_t pos = 0; pos < n; ++pos) {
            Node node(slots, factory);
            storage.loadVector(static_cast<long long>(pos), node);

            const std::size_t id = node.getId();
            if (!g.nodes_.emplace(id, std::move(node)).second) {
                throw std::runtime_error("ClusterGraph::load: duplicate id in cluster: "
                                         + std::to_string(id));
            }
            g.ids_.push_back(id);
        }
        return g;
    }

    ClusterGraph::ClusterGraph(std::size_t slots) : slots_(slots) {}

    void ClusterGraph::save(LinearStorage<Node>& storage)
    {
        const std::size_t n = ids_.size();
        for (std::size_t pos = 0; pos < n; ++pos) {
            storage.overwriteVector(static_cast<long long>(pos), nodes_.at(ids_[pos]));
        }
        storage.commit();
    }

    void ClusterGraph::randomizeNeighbors(std::size_t R,
                                          std::optional<std::uint64_t> seed)
    {
        const std::size_t n = ids_.size();
        if (n < 2 || R == 0) return;

        R = std::min(R, slots_);
        const std::size_t k = std::min(R, n - 1);   // ne moze vise od n-1 razlicitih

        std::mt19937_64 rng(seed ? *seed : std::random_device{}());
        std::uniform_int_distribution<std::size_t> dist(0, n - 1);

        std::unordered_set<std::size_t> chosen;
        std::vector<std::size_t> neighbors;
        neighbors.reserve(slots_);

        for (std::size_t pos = 0; pos < n; ++pos) {
            chosen.clear();
            neighbors.clear();

            while (neighbors.size() < k) {
                const std::size_t p = dist(rng);   
                if (p == pos) continue;             
                if (chosen.insert(p).second)       
                    neighbors.push_back(ids_[p]);  
            }
            neighbors.resize(slots_, EMPTY_NEIGHBOR);
            nodes_.at(ids_[pos]).setNeighbors(neighbors);
        }
    }

    std::size_t ClusterGraph::size() const { return ids_.size(); }
    std::size_t ClusterGraph::slots() const { return slots_; }
    const std::vector<std::size_t>& ClusterGraph::ids() const { return ids_; }
    bool ClusterGraph::contains(std::size_t id) const { return nodes_.count(id) != 0; }
    Node& ClusterGraph::node(std::size_t id) { return nodes_.at(id); }
    const Node& ClusterGraph::node(std::size_t id) const { return nodes_.at(id); }

    double ClusterGraph::dist(std::size_t a, std::size_t b, metrics::IMetrics& distance) const {
        return distance(nodes_.at(a).getVector(), nodes_.at(b).getVector());
    }

    std::size_t ClusterGraph::findMedoid(std::size_t sampleSize, metrics::IMetrics& distance) const {
        std::vector<std::size_t> sample = ids_;
        std::mt19937_64 rng(std::random_device{}());
        std::shuffle(sample.begin(), sample.end(), rng);
        sample.resize(std::min(sampleSize, sample.size()));

        std::size_t best = sample[0];
        double bestSum = std::numeric_limits<double>::max();
        for (std::size_t a : sample) {
            double s = 0;
            for (std::size_t b : sample) s += dist(a, b, distance);
            if (s < bestSum) { bestSum = s; best = a; }
        }
        return best;
    }

    void ClusterGraph::buildPass(std::size_t medoid, double alpha, std::size_t Numb, std::size_t L,
                             metrics::IMetrics& distance)
    {
        const std::size_t n = ids_.size();
        if (n == 0) return;
        const std::size_t R = std::min(Numb, slots_);

        // ---------- 0. gusta kopija grafa (pozicije umesto id-eva) ----------
        std::unordered_map<std::size_t, uint32_t> idToPos;
        idToPos.reserve(n * 2);
        for (std::size_t pos = 0; pos < n; ++pos)
            idToPos.emplace(ids_[pos], static_cast<uint32_t>(pos));

        std::vector<const data::IVector*>  vecs(n);
        std::vector<std::vector<uint32_t>> adj(n);
        for (std::size_t pos = 0; pos < n; ++pos) {
            const Node& node = nodes_.at(ids_[pos]);
            vecs[pos] = &node.getVector();
            adj[pos].reserve(slots_);
            for (std::size_t nb : node.getNeighbors()) {
                if (nb == EMPTY_NEIGHBOR) continue;
                auto it = idToPos.find(nb);
                if (it != idToPos.end()) adj[pos].push_back(it->second);
            }
        }
        const uint32_t medoidPos = idToPos.at(medoid);
        auto d = [&](uint32_t a, uint32_t b) -> double { return distance(*vecs[a], *vecs[b]); };

        // ---------- greedy search: rezultat u s.visited ----------
        auto search = [&](uint32_t target, Scratch& s) {
            s.newEpoch();
            s.list.clear();
            s.visited.clear();
            s.list.push_back({ d(medoidPos, target), medoidPos });
            s.mark[medoidPos] = s.epoch;

            std::size_t k = 0;
            while (true) {
                while (k < s.list.size() && s.list[k].expanded) ++k;
                if (k == s.list.size()) break;

                s.list[k].expanded = true;
                const uint32_t u = s.list[k].id;
                s.visited.push_back({ s.list[k].dist, u });

                std::size_t leftmost = k;
                for (uint32_t nb : adj[u]) {
                    if (s.mark[nb] == s.epoch) continue;
                    s.mark[nb] = s.epoch;
                    const double dn = d(nb, target);
                    if (s.list.size() >= L && dn >= s.list.back().dist) continue;

                    auto it = std::lower_bound(s.list.begin(), s.list.end(), dn,
                                [](const DenseCand& c, double v) { return c.dist < v; });
                    const std::size_t at = static_cast<std::size_t>(it - s.list.begin());
                    s.list.insert(it, { dn, nb });
                    if (s.list.size() > L) s.list.pop_back();
                    leftmost = std::min(leftmost, at);
                }
                k = leftmost;
            }
        };

        // ---------- robust prune: kandidati = s.visited + trenutna lista od p; rezultat u s.out ----------
        auto prune = [&](uint32_t p, Scratch& s) {
            auto& cand = s.visited;
            s.newEpoch();
            s.mark[p] = s.epoch;
            std::size_t w = 0;
            for (std::size_t i = 0; i < cand.size(); ++i)
                if (s.mark[cand[i].id] != s.epoch) { s.mark[cand[i].id] = s.epoch; cand[w++] = cand[i]; }
            cand.resize(w);
            for (uint32_t nb : adj[p])
                if (s.mark[nb] != s.epoch) { s.mark[nb] = s.epoch; cand.push_back({ d(p, nb), nb }); }
            std::sort(cand.begin(), cand.end(), byDist);

            s.out.clear();
            while (!cand.empty() && s.out.size() < R) {
                const DenseCand best = cand.front();
                s.out.push_back(best.id);
                std::size_t keep = 0;
                for (std::size_t i = 1; i < cand.size(); ++i)
                    if (!(alpha * d(best.id, cand[i].id) <= cand[i].dist)) cand[keep++] = cand[i];
                cand.resize(keep);
            }
        };

        // ---------- redosled i batch-evi ----------
        std::vector<uint32_t> order(n);
        std::iota(order.begin(), order.end(), 0u);
        std::mt19937_64 rng(std::random_device{}());
        std::shuffle(order.begin(), order.end(), rng);

        tbb::enumerable_thread_specific<Scratch> tls([&] { return Scratch(n); });
        const std::size_t B = std::clamp<std::size_t>(n / 100, 256, 4096);
        std::vector<std::vector<uint32_t>> fresh(B);
        std::vector<std::pair<uint32_t, uint32_t>> back;      // (ciljni node j, izvor p)
        std::vector<std::size_t> starts;

        for (std::size_t b0 = 0; b0 < n; b0 += B) {
            const std::size_t b1 = std::min(n, b0 + B);

            // ===== FAZA 1 (paralelno): search + JEDAN prune za svaki node iz batch-a =====
            tbb::parallel_for(tbb::blocked_range<std::size_t>(b0, b1),
                [&](const tbb::blocked_range<std::size_t>& r) {
                    Scratch& s = tls.local();
                    for (std::size_t i = r.begin(); i < r.end(); ++i) {
                        search(order[i], s);
                        prune(order[i], s);
                        fresh[i - b0] = s.out;
                    }
                });

            // ===== FAZA 2 (paralelno): objavi nove liste =====
            tbb::parallel_for(tbb::blocked_range<std::size_t>(b0, b1),
                [&](const tbb::blocked_range<std::size_t>& r) {
                    for (std::size_t i = r.begin(); i < r.end(); ++i)
                        adj[order[i]] = std::move(fresh[i - b0]);
                });

            // ===== FAZA 3: skupi povratne grane i grupisi ih po ciljnom nodu =====
            back.clear();
            for (std::size_t i = b0; i < b1; ++i)
                for (uint32_t j : adj[order[i]]) back.push_back({ j, order[i] });
            tbb::parallel_sort(back.begin(), back.end());

            starts.clear();
            for (std::size_t i = 0; i < back.size(); ++i)
                if (i == 0 || back[i].first != back[i - 1].first) starts.push_back(i);
            starts.push_back(back.size());

            // ===== FAZA 4 (paralelno, po grupi): node raste, pa JEDAN prune ako preraste slots_ =====
            tbb::parallel_for(std::size_t(0), starts.size() - 1, [&](std::size_t g) {
                Scratch& s = tls.local();
                const uint32_t j = back[starts[g]].first;
                auto& nj = adj[j];

                s.newEpoch();
                for (uint32_t x : nj) s.mark[x] = s.epoch;
                for (std::size_t i = starts[g]; i < starts[g + 1]; ++i) {
                    const uint32_t p = back[i].second;
                    if (s.mark[p] != s.epoch) { s.mark[p] = s.epoch; nj.push_back(p); }
                }
                if (nj.size() > slots_) {
                    s.visited.clear();
                    prune(j, s);
                    nj = s.out;
                }
            });
        }

        // ---------- finalno: sve liste vece od R se skrate na R ----------
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, n),
            [&](const tbb::blocked_range<std::size_t>& r) {
                Scratch& s = tls.local();
                for (std::size_t p = r.begin(); p < r.end(); ++p) {
                    if (adj[p].size() <= R) continue;
                    s.visited.clear();
                    prune(static_cast<uint32_t>(p), s);
                    adj[p] = s.out;
                }
            });

        // ---------- upis nazad u Node-ove ----------
        for (std::size_t pos = 0; pos < n; ++pos) {
            std::vector<std::size_t> out;
            out.reserve(slots_);
            for (uint32_t nb : adj[pos]) out.push_back(ids_[nb]);
            out.resize(slots_, EMPTY_NEIGHBOR);
            nodes_.at(ids_[pos]).setNeighbors(std::move(out));
        }
    }
}