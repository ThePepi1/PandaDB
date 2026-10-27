#include "../includes/StreamTransletor.h"
#include <memory>
namespace DataStructures {
    StreamTransletor::StreamTransletor(std::unique_ptr<IVectorStream> vectorStream, long long neighborsSize): vectorStream(std::move(vectorStream)), neighborsSize(neighborsSize) {}
    
    bool StreamTransletor::hasNext() {
        return vectorStream->hasNext();
    }
    void StreamTransletor::reset() {
        this->currentIndex = 0;
        vectorStream->reset();
    }

    std::shared_ptr<Node> StreamTransletor::next() {
        this->currentIndex++;
        return std::make_shared<Node>(this->currentIndex,vectorStream->next()->clone(0, this->getDim()), this->neighborsSize);
    }
    int StreamTransletor::getDim() const {
        return vectorStream->getDim();
    }
    long long StreamTransletor::getSize() const {
        return vectorStream->getSize();
    }
    std::vector<std::shared_ptr<Node>> StreamTransletor::getVectors(size_t count) {
        std::vector<std::shared_ptr<Node>> nodes;
        for (size_t i = 0; i < count && this->hasNext(); i++) {
            nodes.push_back(this->next());
        }
        return nodes;
    }
    
}