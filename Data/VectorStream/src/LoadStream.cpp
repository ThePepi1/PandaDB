#include "../includes/LoadStream.h"
#include "Vector/IVector.h"
#include "../../Data/Vamana/includes/Node.h"
#include <algorithm>

namespace DataStructures {

    template<typename T>
    LoadStream<T>::LoadStream(size_t start, size_t end, IStream<T>& stream)
        : currentIndex(0), end(end - start), dim(stream.getDim()) {
        loadedVectors = stream.getVectors(end - start);
    }

    template<typename T>
    bool LoadStream<T>::hasNext() {
        return currentIndex < end;
    }

    template<typename T>
    std::shared_ptr<T> LoadStream<T>::next() {
        if (!hasNext()) {
            return nullptr;
        }
        return loadedVectors[currentIndex++];
    }

    template<typename T>
    void LoadStream<T>::reset() {
        currentIndex = 0;
    }

    template<typename T>
    int LoadStream<T>::getDim() const {
        return dim;
    }

    template<typename T>
    std::vector<std::shared_ptr<T>> LoadStream<T>::getVectors(size_t count) {
        if (count == end) {
            return loadedVectors;
        }
        std::vector<std::shared_ptr<T>> result;
        result.reserve(std::min(count, end - currentIndex));
        for (size_t i = 0; i < count && hasNext(); ++i) {
            result.push_back(next());
        }
        return result;
    }

    template<typename T>
    long long LoadStream<T>::getSize() const {
        return static_cast<long long>(end);
    }

    // ---- Explicit instantiations ----
    template class LoadStream<data::IVector>;
    template class LoadStream<Node>;

}