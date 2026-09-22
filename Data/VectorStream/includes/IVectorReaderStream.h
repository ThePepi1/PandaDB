#pragma once
#include "IVectorStream.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"

namespace DataStructures {
    class IVectorReaderStream : public IVectorStream { 
        private:
            size_t currentIndex;
            size_t totalVectors;
            const LinearStorage& storage;
            data::FactoryVector &factory;
        public:
        ~IVectorReaderStream() override;
        IVectorReaderStream(size_t dim, size_t totalVectors, const LinearStorage& storage, data::FactoryVector& factory);
        std::vector<std::shared_ptr<data::IVector>> getVectors(size_t count) override ;
        bool hasNext() override;
        std::shared_ptr<data::IVector> next() override;
        void reset() override;
        int getDim() const override;

    };
}