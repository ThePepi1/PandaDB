#include "../includes/linearStorage.h"
#include "../../File/includes/FileMenager.h"
#include "../../Data/VectorStream/includes/ReaderStream.h"
#include "../../Data/VectorStream/includes/LoadStream.h"
#include <stdexcept>
#include <memory>
#include <vector>

namespace DataStructures {

    template<typename T>
    LinearStorage<T>::LinearStorage(const std::string& filePath, size_t dim, long long size, size_t extra)
        : filePath(filePath), dim(dim), extra(extra), size(size) {
            file::FileMenager& menager = file::FileMenager::getInstance();
            auto writer = menager.subbscribeFileReaderWriter(filePath);
            if (!writer) {
                throw std::runtime_error("Failed to open file for writing: " + filePath);
            }
            *writer << dim;
            *writer << size;
            writer->commit();
    }

    template<typename T>
    LinearStorage<T>::LinearStorage(const std::string& filePath, size_t extra)
        : filePath(filePath), extra(extra) {
        file::FileMenager& menager = file::FileMenager::getInstance();
        auto reader = menager.subbscribeFileReader(filePath);
        if (!reader) {
            throw std::runtime_error("Failed to open file for reading: " + filePath);
        }
        reader->seek(0);
        *reader >> dim;
        *reader >> size;
    }

    template<typename T>
    size_t LinearStorage<T>::getDim() const {
        return dim;
    }

    template<typename T>
    long long LinearStorage<T>::getSize() const {
        return size;
    }

    template<typename T>
    std::string LinearStorage<T>::getFilePath() const {
        return filePath;
    }

    template<typename T>
    LinearStorage<T>& LinearStorage<T>::saveVector(const T& item, bool commit) {
        file::FileMenager& menager = file::FileMenager::getInstance();
        auto writer = menager.subbscribeFileReaderWriter(filePath);
        if (!writer) {
            throw std::runtime_error("Failed to open file for writing: " + filePath);
        }
        writer->seek(writer->getFileSize());
        *writer << item;
        writer->seek(sizeof(size_t));
        *writer << (++size);
        if (commit) {
            writer->commit();
        }
        return *this;
    }

    template<typename T>
    LinearStorage<T>& LinearStorage<T>::saveVector(const T& item) {
        return saveVector(item, true);
    }

    template<typename T>
    const LinearStorage<T>& LinearStorage<T>::loadVector(long long index, T& item) const {
        auto reader = file::FileMenager::getInstance().subbscribeFileReader(filePath);
        if (!reader) {
            throw std::runtime_error("Failed to open file for reading: " + filePath);
        }
        long long offset = getVectorOffset(index);
        reader->seek(offset);
        *reader >> item;
        return *this;
    }

    template<typename T>
    long long LinearStorage<T>::getVectorOffset(long long index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Index out of range.");
        }
        return sizeof(size_t) + sizeof(long long) + index * static_cast<long long>(Traits::recordSize(dim, extra));
    }

    template<typename T>
    std::unique_ptr<IStream<T>> LinearStorage<T>::getVectorStream(FactoryType& factory) const {
        return std::make_unique<ReaderStream<T>>(dim, 0, size, *this, factory, extra);
    }

    template<typename T>
    std::unique_ptr<IStream<T>> LinearStorage<T>::getVectorStream(size_t start, size_t end, FactoryType& factory) const {
        if (start >= end || end > static_cast<size_t>(size)) {
            throw std::out_of_range("Invalid range for vector stream.");
        }
        return std::make_unique<ReaderStream<T>>(dim, start, end, *this, factory, extra);
    }

    template<typename T>
    std::unique_ptr<IStream<T>> LinearStorage<T>::getVectorStream(double proportion, FactoryType& factory) const {
        long long end = static_cast<long long>(size * proportion);
        return std::make_unique<LoadStream<T>>(0, end, *getVectorStream(0, end, factory));
    }
    template<typename T>
    std::vector<std::unique_ptr<T>> LinearStorage<T>::loadBeam(
        const std::vector<long long>& indices, 
        FactoryType* factory,
        size_t extra
    ) const 
    {
        const size_t numIndices = indices.size();
        std::vector<std::unique_ptr<T>> result;
        if (numIndices == 0) return result;

        result.reserve(numIndices);

        std::vector<char*> buffers(numIndices);
        std::vector<size_t> positions(numIndices);
        std::vector<size_t> sizes(numIndices);

        const size_t recordSizeBytes = Traits::recordSize(dim, extra);

        for (size_t i = 0; i < numIndices; ++i) {
            long long idx = indices[i];
            if (idx < 0 || idx >= size) {
                throw std::out_of_range("Index out of bounds in loadBeam: " + std::to_string(idx));
            }

            // Allocate object memory depending on whether T is abstract or concrete
            std::unique_ptr<T> itemPtr;
            if constexpr (std::is_abstract<T>::value) {
                if (!factory) {
                    throw std::invalid_argument("Factory pointer required when loading abstract types (data::IVector).");
                }
                itemPtr = factory->createVector(dim);
            } else {
                itemPtr = std::make_unique<T>(extra, *factory);
            }

            buffers[i]   = reinterpret_cast<char*>(itemPtr.get());
            positions[i] = static_cast<size_t>(getVectorOffset(idx));
            sizes[i]     = recordSizeBytes;

            result.push_back(std::move(itemPtr));
        }

        // Acquire file reader instance
        auto reader = file::FileMenager::getInstance().subbscribeFileReader(filePath);
        if (!reader) {
            throw std::runtime_error("Failed to open file reader for beam search: " + filePath);
        }

        // Execute parallel async io_uring beam search
        reader->beam(buffers, positions, sizes);

        return result;
    }
    // ---- Explicit instantiations ----
    template class LinearStorage<data::IVector>;
    template class LinearStorage<Node>;

}