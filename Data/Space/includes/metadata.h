#pragma once
#include <cstddef>
#include "../../File/includes/fileHendler.h"
namespace DataStructures {
    struct Metadata { 
        std::size_t vectorDim;
        std::size_t NeighborsSize; // Number of neighbors to store for each vector
        std::size_t beamSize; // Number of candidates to consider during search
        std::size_t K; // number of clusters to assign vectors
        std::size_t L; // Number of candidates generated during index Building by Greedy Search
        std::size_t R; // Number of candidates generated during index Building by Random 
        std::size_t Ef;// Number of candidates generated during search it can change if K is bigger than Ef`
        std::size_t NumberOfClasters;
        std::size_t NumberOfIterations;
        std::size_t numberOfClasters;
        double alpha;
        double TrainingProportion; 
        std::string metrixSystem;
        friend file::fileReader &operator>>(file::fileReader& reader, Metadata& metadata);
        friend file::fileReaderWriter &operator<<(file::fileReaderWriter& writer, const Metadata& metadata);
        friend file::fileReaderWriter &operator>>(file::fileReaderWriter& rw, Metadata& metadata) ;
    }; 
}