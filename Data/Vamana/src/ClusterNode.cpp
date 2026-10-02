#include "../includes/ClusterNode.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace DataStructures {

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
            storage.overwriteVector(static_cast<long long>(pos),
                                    nodes_.at(ids_[pos]),
                                    /*commit=*/ pos + 1 == n);
        }
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
                const std::size_t p = dist(rng);    // uzorkuje se pozicija u ids_
                if (p == pos) continue;             // bez self-loop-a
                if (chosen.insert(p).second)        // bez duplikata
                    neighbors.push_back(ids_[p]);   // upisuje se pravi id
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
    double ClusterGraph::dist(std::size_t a, std::size_t b, metrics::IMetrics& distance) const {
    return distance(nodes_.at(a).getVector(), nodes_.at(b).getVector());
}
    namespace { 
        bool compareCandidates(const ClusterGraph::Candidate& lhs, const ClusterGraph::Candidate& rhs) {
            return lhs.dist < rhs.dist;
        }
    }
       std::vector<ClusterGraph::Candidate>
    ClusterGraph::greedySearch(std::size_t start, std::size_t target,
                               std::size_t L, metrics::IMetrics& distance) const
    {
        std::vector<Candidate> list{ { dist(start, target, distance), start } }; // sortirana, <= L
        std::unordered_set<std::size_t> seen{ start }, expanded;
        std::vector<Candidate> visited;
 
        while (true) {
            // najblizi jos neprosireni kandidat
            auto it = std::find_if(list.begin(), list.end(),
                          [&](const Candidate& c) { return !expanded.count(c.id); });
            if (it == list.end()) break;
 
            const Candidate cur = *it;
            expanded.insert(cur.id);
            visited.push_back(cur);
 
            for (std::size_t nb : nodes_.at(cur.id).getNeighbors()) {
                if (nb == EMPTY_NEIGHBOR || !seen.insert(nb).second) continue;
                list.push_back({ dist(nb, target, distance), nb });
            }
            std::sort(list.begin(), list.end(), compareCandidates);
            if (list.size() > L) list.resize(L);
        }
        return visited;
    }
    void ClusterGraph::robustPrune(std::size_t p, std::vector<Candidate> cand, double alpha,
                                   std::size_t R, metrics::IMetrics& distance)
    {
        // dodaj trenutne susede, izbaci p i duplikate
        for (std::size_t nb : nodes_.at(p).getNeighbors())
            if (nb != EMPTY_NEIGHBOR) cand.push_back({ dist(p, nb, distance), nb });
 
        cand.erase(std::remove_if(cand.begin(), cand.end(),
                       [&](const Candidate& c) { return c.id == p; }), cand.end());
        std::sort(cand.begin(), cand.end(), compareCandidates);
        cand.erase(std::unique(cand.begin(), cand.end(),
                       [](const Candidate& a, const Candidate& b) { return a.id == b.id; }),
                   cand.end());
 
        std::vector<std::size_t> out;
        out.reserve(slots_);
        while (!cand.empty() && out.size() < R) {
            const Candidate best = cand.front();   // najblizi tacki p
            out.push_back(best.id);
            // izbaci sve koje `best` "pokriva" (ukljucujuci i sam `best`, jer je d(best,best)=0)
            cand.erase(std::remove_if(cand.begin(), cand.end(), [&](const Candidate& c) {
                return alpha * dist(best.id, c.id, distance) <= c.dist;
            }), cand.end());
        }
        out.resize(slots_, EMPTY_NEIGHBOR);
        nodes_.at(p).setNeighbors(std::move(out));
    }
    void ClusterGraph::buildPass(std::size_t medoid, double alpha, std::size_t Numb, std::size_t L,metrics::IMetrics& distance)
    {
        std::vector<std::size_t> order = ids_;
        std::mt19937_64 rng(std::random_device{}());
        std::shuffle(order.begin(), order.end(), rng);
 
        for (std::size_t p : order) {
            robustPrune(p, greedySearch(medoid, p, L, distance), alpha, Numb, distance);
 
            // povratne grane (kopija, jer setNeighbors menja listu)
            const std::vector<std::size_t> outs = nodes_.at(p).getNeighbors();
            for (std::size_t j : outs) {
                if (j == EMPTY_NEIGHBOR) continue;
 
                std::vector<std::size_t> nj = nodes_.at(j).getNeighbors();
                if (std::find(nj.begin(), nj.end(), p) != nj.end()) continue;  // vec postoji
 
                auto freeSlot = std::find(nj.begin(), nj.end(), EMPTY_NEIGHBOR);
                if (freeSlot != nj.end() &&
                    static_cast<std::size_t>(freeSlot - nj.begin()) < Numb) {  // ima mesta: samo dodaj
                    *freeSlot = p;                                  // ima mesta: samo dodaj
                    nodes_.at(j).setNeighbors(std::move(nj));
                } else {
                    // pun: prune(N(j) ∪ {p})
                    robustPrune(j, { { dist(j, p, distance), p } }, alpha, Numb, distance);
                }
            }
        }
    }
}