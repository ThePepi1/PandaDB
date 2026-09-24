#include "../includes/IVectorReaderStream.h"

namespace DataStructures {
    IVectorReaderStream::IVectorReaderStream(size_t dim, size_t start, size_t end, const LinearStorage& storage, data::FactoryVector& factory)
        : currentIndex(start), end(end), start(start), storage(storage), factory(factory) {
    }
    std::vector<std::shared_ptr<data::IVector>> IVectorReaderStream::getVectors(size_t count) {
        std::vector<std::shared_ptr<data::IVector>> vectors;
        for (size_t i = 0; i < count && hasNext(); ++i) {
            vectors.push_back(next());
        }
        return vectors;
    }
    bool IVectorReaderStream::hasNext() {
        return currentIndex < end;
    }
    std::shared_ptr<data::IVector> IVectorReaderStream::next() {
        if (!hasNext()) {
            throw std::out_of_range("No more vectors in the stream.");
        }
        std::shared_ptr<data::IVector> vector = factory.createVector(storage.getDim()); 
        storage.loadVector(currentIndex, *vector);
        ++currentIndex;
        return vector;
    }
    void IVectorReaderStream::reset() {
        currentIndex = start;
    }
    int IVectorReaderStream::getDim() const {
        return storage.getDim();
    }
    long long IVectorReaderStream::getSize() const {
        return end - start;
    }
}