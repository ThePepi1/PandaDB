#pragma once
#include "linearStorageTraits.h"
#include "../../Data/VectorStream/includes/IStream.h"
#include "../../File/includes/FileMenager.h"
#include <memory>
#include <string>
#include <vector>

namespace DataStructures {

enum class OpenMode { Read, Write };

template<typename T>
class LinearStorage {
public:
    using Traits      = LinearStorageTraits<T>;
    using FactoryType = typename Traits::FactoryType;

private:
    using ReaderHandle = std::unique_ptr<file::fileReader, file::fileReader::Deleter>;
    using WriterHandle = std::unique_ptr<file::fileReaderWriter, file::fileReaderWriter::Deleter>;

    std::string  filePath;
    OpenMode     mode;
    size_t       dim   = 0;
    size_t       extra = 0;
    long long    size  = 0;

    ReaderHandle reader;            // set only in Read mode
    WriterHandle writer;            // set only in Write mode
    bool headerDirty = false;
    bool dirty       = false;

    long long getVectorOffset(long long index) const;
    void requireWrite(const char* op) const;
    void writeHeader();
    file::fileReader& io() const;   // picks reader or writer by mode

public:
    LinearStorage(const std::string& filePath, size_t dim, long long size, size_t extra = 0);
    LinearStorage(const std::string& filePath, OpenMode mode, bool hasExtra = false);

    LinearStorage(const LinearStorage&) = delete;
    LinearStorage& operator=(const LinearStorage&) = delete;
    LinearStorage(LinearStorage&&) noexcept = default;
    LinearStorage& operator=(LinearStorage&&) noexcept = default;
    ~LinearStorage();

    OpenMode    getMode() const { return mode; }
    bool        isWritable() const { return mode == OpenMode::Write; }
    size_t      getDim() const;
    size_t      getExtra() const;
    long long   getSize() const;
    std::string getFilePath() const;

    LinearStorage& saveVector(const T& item);
    LinearStorage& overwriteVector(long long index, const T& item);
    void commit();

    const LinearStorage& loadVector(long long index, T& item) const;
    std::vector<std::unique_ptr<T>> loadBeam(const std::vector<long long>& indices,
                                             FactoryType* factory, size_t extra = 0) const;
    std::unique_ptr<IStream<T>> getVectorStream(FactoryType& factory) const;
    std::unique_ptr<IStream<T>> getVectorStream(size_t startIndex, size_t endIndex, FactoryType& factory) const;
    std::unique_ptr<IStream<T>> getVectorStream(double proportion, FactoryType& factory) const;
};

}