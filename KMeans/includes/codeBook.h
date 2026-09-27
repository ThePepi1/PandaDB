#pragma once
#include "../includes/Vector/IVector.h"
#include "../includes//Vector/FactoryVector.h"
#include "../../Data/VectorStream/includes/VectorStreams.h"
#include "../../Metrics/includes/IMetrics.h"
#include <memory>
#include <mutex>
#include <vector>
#include "../../File/includes/fileHendler.h"
namespace KMeans {
    class Trainer;
    class CodeBook {
    private:
        long long coodeBookSize;
        int codeBookDim;
        data::FactoryVector& factory;
        std::vector<std::unique_ptr<data::IVector>> centroids;
        std::vector<std::unique_ptr<data::IVector>> averages;
        std::vector<long long> counts;
        std::vector<std::mutex> centroidMutexes ;
    public:
        CodeBook(int size, int dim, data::FactoryVector& factory);
        CodeBook(int size, int dim, data::FactoryVector& factory, bool generate);
        /*
            idea behind not changing metics is that we are here saving centeroids u 
            could use difrent metrics for traning and fiting this just saves centeroids
        */
        long long belongs(const data::IVector& vector, const metrics::IMetrics& metrics) const; 
        std::vector<long long> belongs(const data::IVector& vector, const metrics::IMetrics& metrics, int k) const;
        void fit(data::IVector& point, const metrics::IMetrics& metrics);
        bool updateAverages();
        friend file::fileReader &operator>>(file::fileReader& reader, CodeBook& codeBook);
        friend file::fileReaderWriter &operator<<(file::fileReaderWriter& writer, const CodeBook& codeBook);
        friend file::fileReaderWriter &operator>>(file::fileReaderWriter& rw, CodeBook& codeBook);
        int getDim() const;
        long long getSize() const;
        const data::IVector& getCentroid(long long index) const;
        void train(DataStructures::IVectorStream& stream, const metrics::IMetrics& metrics, int maxIterations);
        friend class Trainer;
        const data::IVector& operator[](long long index) const;
    };
}
