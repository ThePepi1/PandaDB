#pragma once
#include <memory>
#include <vector>
#include "Vector/FactoryVector.h"
#include "codeBook.h"
namespace KMeans {
    class Trainer;
    class ProductQuantizer {
        private:
            int numSubspaces;
            int numCentroids;
            int dim;
            data::FactoryVector& factory; // idk why this is pointer but it is. Not anymore
            std::vector<std::unique_ptr<CodeBook>> codeBooks;
        public:
            ProductQuantizer(int numSubspaces, int numCentroids, int dim, data::FactoryVector& factory);
            ProductQuantizer(data::FactoryVector &factory); 
            std::vector<long long> quantize(const data::IVector& vector, const metrics::IMetrics& metrics) const;
            const CodeBook& getCodeBook(int index) const;
            const CodeBook& operator[](int index) const;
            int getNumSubspaces() const;
            int getNumCentroids() const;
            int getDim() const;


            friend file::fileReader &operator>>(file::fileReader& reader, ProductQuantizer& pq);
            friend file::fileReaderWriter &operator<<(file::fileReaderWriter& writer, const ProductQuantizer& pq);
            friend file::fileReaderWriter &operator>>(file::fileReaderWriter& rw, ProductQuantizer& pq);
            friend class Trainer;
    };
}