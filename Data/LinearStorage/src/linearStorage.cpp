#include "../includes/linearStorage.h"
#include "../../File/includes/FileMenager.h"
#include "../../Data/VectorStream/includes/ReaderStream.h"
#include "../../Data/VectorStream/includes/LoadStream.h"
#include <stdexcept>
#include <memory>
#include <vector>
#include <string>
#include <type_traits>

namespace DataStructures {


    template<typename T>
    file::fileReader& LinearStorage<T>::io() const {
        if (mode == OpenMode::Write) return *writer;
        return *reader;
    }

    template<typename T>
    void LinearStorage<T>::requireWrite(const char* op) const {
        if (mode != OpenMode::Write || !writer) {
            throw std::logic_error(std::string(op) + " called on a read-only LinearStorage: " + filePath);
        }
    }

    template<typename T>
    void LinearStorage<T>::writeHeader() {
        writer->seek(0);
        *writer << dim;
        *writer << size;
        *writer << extra;
        headerDirty = false;
        dirty = true;
    }

    template<typename T>
    long long LinearStorage<T>::getVectorOffset(long long index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Index out of range.");
        }
        return 2 * sizeof(size_t) + sizeof(long long)
               + index * static_cast<long long>(Traits::recordSize(dim, extra));
    }


    template<typename T>
    LinearStorage<T>::LinearStorage(const std::string& filePath, size_t dim, long long size, size_t extra)
        : filePath(filePath), mode(OpenMode::Write), dim(dim), extra(extra), size(size)
    {
        writer = file::FileMenager::getInstance().subbscribeFileReaderWriter(filePath);
        if (!writer) {
            throw std::runtime_error("Failed to open file for writing: " + filePath);
        }
        writeHeader();
        commit();
    }

    template<typename T>
    LinearStorage<T>::LinearStorage(const std::string& filePath, OpenMode mode, bool hasExtra)
        : filePath(filePath), mode(mode)
    {
        auto& mgr = file::FileMenager::getInstance();

        if (mode == OpenMode::Write) {
            writer = mgr.subbscribeFileReaderWriter(filePath);
            if (!writer) {
                throw std::runtime_error("Failed to open file for writing: " + filePath);
            }
        } else {
            reader = mgr.subbscribeFileReader(filePath);
            if (!reader) {
                throw std::runtime_error("Failed to open file for reading: " + filePath);
            }
        }

        file::fileReader& f = io();
        f.seek(0);
        f >> dim;
        f >> size;
        if (hasExtra) {
            f >> extra;
        } else {
            extra = 0;
        }
    }

    template<typename T>
    LinearStorage<T>::~LinearStorage() {
        if (writer && (dirty || headerDirty)) {
            try { commit(); } catch (...) {}   // never throw from a destructor
        }
    }


    template<typename T>
    size_t LinearStorage<T>::getDim() const { return dim; }

    template<typename T>
    size_t LinearStorage<T>::getExtra() const { return extra; }

    template<typename T>
    long long LinearStorage<T>::getSize() const { return size; }

    template<typename T>
    std::string LinearStorage<T>::getFilePath() const { return filePath; }


    template<typename T>
    LinearStorage<T>& LinearStorage<T>::saveVector(const T& item) {
        requireWrite("saveVector");
        writer->seek(writer->getFileSize());
        *writer << item;
        ++size;
        headerDirty = true;   
        dirty = true;
        return *this;
    }

    template<typename T>
    LinearStorage<T>& LinearStorage<T>::overwriteVector(long long index, const T& item) {
        requireWrite("overwriteVector");
        writer->seek(getVectorOffset(index));   
        dirty = true;
        return *this;
    }

    template<typename T>
    void LinearStorage<T>::commit() {
        requireWrite("commit");
        if (headerDirty) {
            writer->seek(sizeof(size_t));       
            *writer << size;
            headerDirty = false;
            dirty = true;
        }
        if (dirty) {
            writer->commit();
            dirty = false;
        }
    }


    template<typename T>
    const LinearStorage<T>& LinearStorage<T>::loadVector(long long index, T& item) const {
        const long long offset = getVectorOffset(index);   // throws before touching the file
        file::fileReader& f = io();
        f.seek(static_cast<size_t>(offset));
        f >> item;
        return *this;
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

        if (!factory) {
            throw std::invalid_argument("Factory pointer required in loadBeam.");
        }

        result.reserve(numIndices);

        std::vector<char*>  buffers(numIndices);
        std::vector<size_t> positions(numIndices);
        std::vector<size_t> sizes(numIndices);

        const size_t recordSizeBytes = Traits::recordSize(dim, extra);

        for (size_t i = 0; i < numIndices; ++i) {
            long long idx = indices[i];
            if (idx < 0 || idx >= size) {
                throw std::out_of_range("Index out of bounds in loadBeam: " + std::to_string(idx));
            }

            std::unique_ptr<T> itemPtr;
            if constexpr (std::is_abstract<T>::value) {
                itemPtr = factory->createVector(dim);
            } else {
                itemPtr = std::make_unique<T>(extra, *factory);
            }

            buffers[i]   = reinterpret_cast<char*>(itemPtr.get());
            positions[i] = static_cast<size_t>(getVectorOffset(idx));
            sizes[i]     = recordSizeBytes;

            result.push_back(std::move(itemPtr));
        }

        io().beam(buffers, positions, sizes);   

        return result;
    }

    template class LinearStorage<data::IVector>;
    template class LinearStorage<Node>;

}