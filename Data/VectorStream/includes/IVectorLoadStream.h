#pragma once
#include "IVectorStream.h"
#include "../includes/Vector/IVector.h"

namespace DataStructures {
    class IVectorLoadStream : public IVectorStream {
        private:
            size_t currentIndex;
            size_t end;
            std::vector<std::shared_ptr<data::IVector>> loadedVectors;
        public:
        IVectorLoadStream(size_t start, size_t end, IVectorStream& stream) ;
        ~IVectorLoadStream() override = default;
        bool hasNext() override;
        std::shared_ptr<data::IVector> next() override;
        void reset() override;
        int getDim() const override;
        std::vector<std::shared_ptr<data::IVector>> getVectors(size_t count) override;
        long long getSize() const override;
    };
}