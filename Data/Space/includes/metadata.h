#pragma once
#include <cstddef>

namespace DataStructures {
    struct Metadata { 
        std::size_t vectorDim;
        std::size_t NeighborsSize;
        std::size_t beamSize;
        double randmoProportion;
        double alpha;

    }; 
}