#include "../includes/ReaderStream.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"
#include <stdexcept>

namespace DataStructures {

    template<typename T>
    ReaderStream<T>::ReaderStream(size_t dim, size_t start, size_t end,
                                   const LinearStorage<T>& storage, FactoryType& factory, size_t extra)
        : currentIndex(start), end(end), start(start), dim(dim),
          extra(extra), storage(storage), factory(factory) {
    }

    template<typename T>
    std::vector<std::shared_ptr<T>> ReaderStream<T>::getVectors(size_t count) {
        std::vector<std::shared_ptr<T>> items;
        for (size_t i = 0; i < count && hasNext(); ++i) {
            items.push_back(next());
        }
        return items;
    }

    template<typename T>
    bool ReaderStream<T>::hasNext() {
        return currentIndex < end;
    }

    template<typename T>
    std::shared_ptr<T> ReaderStream<T>::next() {
        if (!hasNext()) {
            throw std::out_of_range("No more items in the stream.");
        }
        auto item = Traits::createEmpty(factory, dim, extra);
        storage.loadVector(currentIndex, *item);
        ++currentIndex;
        return item;
    }

    template<typename T>
    void ReaderStream<T>::reset() {
        currentIndex = start;
    }

    template<typename T>
    int ReaderStream<T>::getDim() const {
        return static_cast<int>(dim);
    }

    template<typename T>
    long long ReaderStream<T>::getSize() const {
        return static_cast<long long>(end - start);
    }


    template class ReaderStream<data::IVector>;
    template class ReaderStream<Node>;

}