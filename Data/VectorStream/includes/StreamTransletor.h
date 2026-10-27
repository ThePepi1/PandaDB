#pragma once
#include "../includes/NodeStream.h"
#include "../includes/VectorStreams.h"
#include <memory>

namespace DataStructures {
    class StreamTransletor: public INodeStream {
        private:
            std::unique_ptr<IVectorStream> vectorStream;
            size_t currentIndex = 0;
            long long neighborsSize;
        public:
            StreamTransletor(std::unique_ptr<IVectorStream> vectorStream, long long neighborsSize);
            virtual ~StreamTransletor() = default;
            virtual bool hasNext() override;
            virtual std::shared_ptr<Node> next() override;
            virtual void reset() override;
            virtual int getDim() const override;
            virtual long long getSize() const override;
            virtual std::vector<std::shared_ptr<Node>> getVectors(size_t count) override;
              
    };
}