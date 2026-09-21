#pragma once
#include "../includes/Vector/IVector.h"
#include "../includes//Vector/FactoryVector.h"
#include "../../Data/VectorStream/includes/IVectorStream.h"
#include "../../Metrics/includes/IMetrics.h"
#include <memory>
#include <vector>
#include "../../File/includes/fileHendler.h"
namespace KMeans {
    class CodeBook {
    private:
        long long coodeBookSize;
        int codeBookDim;
        data::FactoryVector* factory;
        std::vector<std::unique_ptr<data::IVector>> centroids;
    public:
        CodeBook(int size, int dim, data::FactoryVector* factory);
        /*
            idea behind not changing metics is that we are here saving centeroids u 
            could use difrent metrics for traning and fiting this just saves centeroids
        */
        long long belongs(const data::IVector& vector, const metrics::IMetrics& metrics) const; 
        bool fit(DataStructures::IVectorStream& stream, const metrics::IMetrics& metrics);
        friend file::fileReader &operator>>(file::fileReader& reader, CodeBook& codeBook);
        friend file::fileReaderWriter &operator<<(file::fileReaderWriter& writer, const CodeBook& codeBook);
        friend file::fileReaderWriter &operator>>(file::fileReaderWriter& rw, CodeBook& codeBook);
        int getDim() const;
        long long getSize() const;

    };
}
