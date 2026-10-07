#pragma once
#include "IStream.h"
#include "../../Data/LinearStorage/includes/linearStorageTraits.h"

namespace DataStructures {

    template<typename T> class LinearStorage;

    template<typename T>
    class ReaderStream : public IStream<T> {
        public:
            using Traits      = LinearStorageTraits<T>;
            using FactoryType = typename Traits::FactoryType;

        private:
            size_t currentIndex;
            size_t end;
            size_t start;
            size_t dim;
            size_t extra;
            const LinearStorage<T>& storage;
            FactoryType& factory;

        public:
            ReaderStream(size_t dim, size_t start, size_t end,
                         const LinearStorage<T>& storage, FactoryType& factory, size_t extra = 0);
            ~ReaderStream() override = default;

            std::vector<std::shared_ptr<T>> getVectors(size_t count) override;
            bool hasNext() override;
            std::shared_ptr<T> next() override;
            void reset() override;
            int getDim() const override;
            long long getSize() const override;
    };

}