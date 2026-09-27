#include "../includes/Node.h"
#include <cstring>
namespace DataStructures{
    const size_t Node::getId() const {
        return this->id;
    }
    const data::IVector& Node::getVector() const {
        return *this->Vector;
    }
    const std::vector<size_t> & Node::getNeighbors() const {
        return this->Neighbors;
    }
    Node::Node(size_t id, std::unique_ptr<data::IVector> Vector): id(id), Vector(std::move(Vector)) {}

    void Node::serialize(char* buffer) const {
        std::memcpy(buffer, &id, sizeof(size_t));
        this->Vector->serialize(buffer + sizeof(size_t));
        std::memcpy(buffer + sizeof(size_t) + this->Vector->getSizeInBytes(this->Vector->Dimension()), &Neighbors[0], Neighbors.size() * sizeof(size_t));
    }
    void Node::deserialize(const char* buffer, data::FactoryVector& factory, size_t neighborsSize) {
        std::memcpy(&id, buffer, sizeof(size_t));
        this->Vector = factory.createVector();
        this->Vector->deserialize(buffer + sizeof(size_t));
        this->Neighbors.resize(neighborsSize);
        std::memcpy(&Neighbors[0], buffer + sizeof(size_t) + this->Vector->getSizeInBytes(this->Vector->Dimension()), neighborsSize * sizeof(size_t));
    }
    size_t Node::getSerializedSize(size_t vectorSize, size_t neighborsSize) {
        return sizeof(size_t) + data::IVector::getSizeInBytes(vectorSize) + neighborsSize * sizeof(size_t);
    }
    file::fileReaderWriter& operator<<(file::fileReaderWriter& file, const Node& node) {
        file << node.id;
        file << (*node.Vector);
        for (const auto& neighbor : node.Neighbors) {
            file << neighbor;
        }
        return file;
    }
    file::fileReader& operator>>(file::fileReader& file,  Node& node) {
        file >> node.id;
        file >> (*node.Vector);
        for (auto& neighbor : node.Neighbors) {
            file >> neighbor;
        }
        return file;
    }
    file::fileReaderWriter& operator>>(file::fileReaderWriter& file,  Node& node) {
        file >> node.id;
        file >> (*node.Vector);
        for (size_t& neighbor : node.Neighbors) {
            file >> neighbor;
        }
        return file;
    }
    Node::Node(size_t neighborsSize, data::FactoryVector& factory)
        : id(0), Vector(factory.createVector()), Neighbors(neighborsSize, 0) {}
    
}