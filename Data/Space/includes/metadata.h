#pragma once
#include <cstddef>

namespace DataStructures {
    struct Metadata { 
        std::size_t vectorDim;
        std::size_t NeighborsSize; // Number of neighbors to store for each vector
        std::size_t beamSize; // Number of candidates to consider during search
        std::size_t K; // number of clusters to assign vectors
        std::size_t L; // Number of candidates generated during index Building by Greedy Search
        std::size_t R; // Number of candidates generated during index Building by Random
        std::size_t Ef;// Number of candidates generated during search it can change if K is bigger than Ef`
        double alpha;

    }; 
}