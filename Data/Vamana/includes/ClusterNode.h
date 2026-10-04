#pragma once
#include "Node.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "../includes/Vector/FactoryVector.h"
#include "../../Metrics/includes/IMetrics.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

namespace DataStructures {

    inline constexpr std::size_t EMPTY_NEIGHBOR = std::numeric_limits<std::size_t>::max();

    class ClusterGraph {
    public:
        static ClusterGraph load(LinearStorage<Node>& storage, data::FactoryVector& factory, std::size_t slots);

        void save(LinearStorage<Node>& storage);

        void randomizeNeighbors(std::size_t R, std::optional<std::uint64_t> seed = std::nullopt);
        std::size_t size() const;
        std::size_t slots() const;
        const std::vector<std::size_t>& ids() const;
        bool contains(std::size_t id) const;
        Node& node(std::size_t id);
        const Node& node(std::size_t id) const;
        double dist(std::size_t a, std::size_t b, metrics::IMetrics& distance) const;
        std::size_t findMedoid(std::size_t sampleSize, metrics::IMetrics& distance) const;
        void buildPass(std::size_t medoid, double alpha, std::size_t R, std::size_t L,
                       metrics::IMetrics& distance);
    private:
        ClusterGraph(std::size_t slots);
        std::size_t slots_;
        std::vector<std::size_t> ids_;
        std::unordered_map<std::size_t, Node> nodes_;
    };

}