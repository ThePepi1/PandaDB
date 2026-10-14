#include "../includes/metadata.h"
namespace DataStructures {
    file::fileReader &operator>>(file::fileReader& reader, Metadata& metadata) {
            reader >> metadata.vectorDim;
            reader >> metadata.NeighborsSize;
            reader >> metadata.beamSize;
            reader >> metadata.K;
            reader >> metadata.L;
            reader >> metadata.R;
            reader >> metadata.Ef;
            reader >> metadata.alpha;
            reader >> metadata.metrixSystem;
            reader >> metadata.NumberOfClasters;
            reader >> metadata.NumberOfIterations;
            reader >> metadata.TrainingProportion;
            return reader;
    }
    file::fileReaderWriter &operator<<(file::fileReaderWriter& writer, const Metadata& metadata) {
            writer << metadata.vectorDim;
            writer << metadata.NeighborsSize;
            writer << metadata.beamSize;
            writer << metadata.K;
            writer << metadata.L;
            writer << metadata.R;
            writer << metadata.Ef;
            writer << metadata.alpha;
            writer << metadata.metrixSystem;
            writer << metadata.NumberOfClasters;
            writer << metadata.NumberOfIterations;
            writer << metadata.TrainingProportion;
            return writer;
}
    file::fileReaderWriter &operator>>(file::fileReaderWriter& rw, Metadata& metadata) { 
    rw >> metadata.vectorDim;
    rw >> metadata.NeighborsSize;
    rw >> metadata.beamSize;
    rw >> metadata.K;
    rw >> metadata.L;
    rw >> metadata.R;
    rw >> metadata.Ef;
    rw >> metadata.alpha;
    rw >> metadata.metrixSystem;
    rw >> metadata.NumberOfClasters;
    rw >> metadata.NumberOfIterations;
    rw >> metadata.TrainingProportion;

    return rw;
}

}