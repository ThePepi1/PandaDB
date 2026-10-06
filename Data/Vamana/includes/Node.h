#pragma once
#include "../includes/Vector/IVector.h"
#include "../../File/includes/fileHendler.h"
#include <cstddef>
#include <memory>
#include <vector>
#include <../includes/Vector/FactoryVector.h>


namespace DataStructures{ 
    class Node
    {
    size_t id;    
    std::unique_ptr<data::IVector> Vector;
    std::vector<size_t> Neighbors; //FixedSize of Neighbours fast access to them for all in mine index :D
    
    public:
        Node(size_t id, std::unique_ptr<data::IVector> Vector, size_t neighborsSize);
        Node(size_t neighborsSize, data::FactoryVector& factory); 
        const size_t getId() const;
        const data::IVector& getVector() const;
        const std::vector<size_t>& getNeighbors() const;
        void setID(size_t id);
        void setVector(const data::IVector& vector);
        friend file::fileReader& operator>>(file::fileReader& file,  Node& node);
        friend file::fileReaderWriter& operator>>(file::fileReaderWriter& file,  Node& node);
        friend file::fileReaderWriter& operator<<(file::fileReaderWriter& file, const Node& node);
        void serialize(char* buffer) const;
        void setNeighbors(std::vector<size_t> neighbors);
        void deserialize(const char* buffer, data::FactoryVector& factory, size_t neighborsSize);
        static size_t getSerializedSize(size_t vectorSize, size_t neighborsSize);
        void extend(Node& other);
    };
}   