#pragma once
#include "IStream.h"
#include <vector>

namespace DataStructures {

    template<typename T>
    class LoadStream : public IStream<T> {
        private:
            size_t currentIndex;
            size_t end;
            int dim;
            std::vector<std::shared_ptr<T>> loadedVectors;

        public:
            LoadStream(size_t start, size_t end, IStream<T>& stream);

            bool hasNext() override;
            std::shared_ptr<T> next() override;
            void reset() override;
            int getDim() const override;
            std::vector<std::shared_ptr<T>> getVectors(size_t count) override;
            long long getSize() const override;
    };

}