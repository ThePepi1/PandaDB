#include "../includes/linearStorage.h"
#include "../../File/includes/FileMenager.h"
namespace DataStructures {
    LinearStorage::LinearStorage(const std::string& filePath, size_t dim, long long size)
        : filePath(filePath), dim(dim), size(size) {
            file::FileMenager& menager = file::FileMenager::getInstance();
            auto writer = menager.subbscribeFileReaderWriter(filePath);
            if (!writer) {
                throw std::runtime_error("Failed to open file for writing: " + filePath);
            }
            *writer << dim;
            *writer << size;
            writer->commit();
        }
    LinearStorage::LinearStorage(const std::string& filePath)
        : filePath(filePath) {
            file::FileMenager& menager = file::FileMenager::getInstance();
            auto reader = menager.subbscribeFileReader(filePath);
            if (!reader) {
                throw std::runtime_error("Failed to open file for reading: " + filePath);
            }
            *reader >> dim;
            *reader >> size;
        }
    size_t LinearStorage::getDim() const {
        return dim;
    }
    long long LinearStorage::getSize() const {
        return size;
    }
    std::string LinearStorage::getFilePath() const {
        return filePath;
    }
    LinearStorage& LinearStorage::saveVector(const data::IVector& vector , bool commit) {
        if (vector.Dimension() != dim) {
            throw std::invalid_argument("Vector dimension does not match storage dimension.");
        }
        file::FileMenager& menager = file::FileMenager::getInstance();
        auto writer = menager.subbscribeFileReaderWriter(filePath);
        if (!writer) {
            throw std::runtime_error("Failed to open file for writing: " + filePath);
        }
        writer->seek(writer->getFileSize());
        *writer << vector;  
        writer->seek(sizeof(size_t)); // Move to the position of the size variable
        *writer << (++size);
        if(commit) {
            writer->commit();
        }
        return *this;
    }
    LinearStorage& LinearStorage::saveVector(const data::IVector& vector) {
        return saveVector(vector, true);
    }
    const LinearStorage& LinearStorage::loadVector(long long index, data::IVector& vector) const  {
       auto wrriter = file::FileMenager::getInstance().subbscribeFileReader(filePath);
        if(!wrriter) {
            throw std::runtime_error("Failed to open file for reading: " + filePath);
        }
        long long offset = getVectorOffset(index);
        wrriter->seek(offset);
        *wrriter >> vector;
        return *this;
    }
    long long LinearStorage::getVectorOffset(long long index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Index out of range.");
        }
        return sizeof(size_t) + sizeof(long long) + index * data::IVector::getSizeInBytes(dim);
    }


}