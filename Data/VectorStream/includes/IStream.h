#pragma once
#include <vector>
#include <memory>

namespace DataStructures {

    template<typename T>
    class IStream {
        public:
            virtual ~IStream() = default;
            virtual std::vector<std::shared_ptr<T>> getVectors(size_t count) = 0;
            virtual bool hasNext() = 0;
            virtual std::shared_ptr<T> next() = 0;
            virtual void reset() = 0;
            virtual int getDim() const = 0;
            virtual long long getSize() const = 0;
    };

}