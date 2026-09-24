#include "../includes/productquntizier.h"
#include <vector>
namespace KMeans {
ProductQuantizer::ProductQuantizer(int numSubspaces, int numCentroids, int dim, data::FactoryVector & factory)
    : numSubspaces(numSubspaces), numCentroids(numCentroids), dim(dim), factory(factory) {
    for (int i = 0; i < numSubspaces - 1; ++i) {
        codeBooks.push_back(std::make_unique<CodeBook>(numCentroids, dim / numSubspaces, factory));
    }
    // Handle the last subspace, which may have a different dimension
    int lastSubspaceDim = dim - (dim / numSubspaces) * (numSubspaces - 1);
    codeBooks.push_back(std::make_unique<CodeBook>(numCentroids, lastSubspaceDim, factory));
}
ProductQuantizer::ProductQuantizer(data::FactoryVector & factory)
    : numSubspaces(0), numCentroids(0), dim(0), factory(factory) {}
const CodeBook& ProductQuantizer::getCodeBook(int index) const {
    if (index < 0 || index >= numSubspaces) {
        throw std::out_of_range("Index out of range in getCodeBook");
    }
    return *codeBooks[index];
}
const CodeBook& ProductQuantizer::operator[](int index) const {
    return getCodeBook(index);
}
std::vector<long long> ProductQuantizer::quantize(const data::IVector& vector, const metrics::IMetrics& metrics) const {
    std::vector<long long> quantizedIndices;
    int subspaceDim = dim / numSubspaces;
    for (int i = 0; i < numSubspaces; ++i) {
        int startIdx = i * subspaceDim;
        int endIdx = (i == numSubspaces - 1) ? dim : startIdx + subspaceDim;
        // Create a subvector for the current subspace it dont dellete double bcs subvector dont have ownership of data
        std::unique_ptr<data::IVector> subVector = factory.createVector(endIdx - startIdx, vector[startIdx]);
        long long index = codeBooks[i]->belongs(*subVector, metrics);
        quantizedIndices.push_back(index);
    }
    return quantizedIndices;
}

int ProductQuantizer::getNumSubspaces() const {
    return numSubspaces;
}
int ProductQuantizer::getNumCentroids() const {
    return numCentroids;
}
int ProductQuantizer::getDim() const {
    return dim;
}
file::fileReader &operator>>(file::fileReader& reader, ProductQuantizer& pq)
{ 
    reader >> pq.numSubspaces;
    reader >> pq.numCentroids;
    reader >> pq.dim;
    pq.codeBooks.clear();
    for (int i = 0; i < pq.numSubspaces; ++i) {
        std::unique_ptr<CodeBook> cb = std::make_unique<CodeBook>(pq.numCentroids, 1, pq.factory, false);
        reader >> *cb;
        pq.codeBooks.push_back(std::move(cb));
    }
    return reader;
}
file::fileReaderWriter &operator<<(file::fileReaderWriter& writer, const ProductQuantizer& pq) {
    writer << pq.numSubspaces;
    writer << pq.numCentroids;
    writer << pq.dim;
    for (const auto& cb : pq.codeBooks) {
        writer << *cb;
    }
    return writer; 
    }
file::fileReaderWriter &operator>>(file::fileReaderWriter& rw, ProductQuantizer& pq) {
    rw >> pq.numSubspaces;
    rw >> pq.numCentroids;
    rw >> pq.dim;
    pq.codeBooks.clear();
    for (int i = 0; i < pq.numSubspaces; ++i) {
        std::unique_ptr<CodeBook> cb = std::make_unique<CodeBook>(pq.numCentroids, 1, pq.factory, false);
        rw >> *cb;
        pq.codeBooks.push_back(std::move(cb));
    }
    return rw;
}
}