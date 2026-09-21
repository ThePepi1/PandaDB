#include "../includes/fileHendler.h"
#include "../includes/FileMenager.h"
#include <filesystem>
#include <cstring>


void file::fileReader::Deleter::operator()(fileReader* reader) const {
    file::FileMenager::getInstance().releseFileReader(reader->filePath);
    delete reader;
}
void file::fileReaderWriter::Deleter::operator()(fileReaderWriter* reader) const {
    file::FileMenager::getInstance().releseFileReaderWriter(reader->filePath);
    delete reader;
}
size_t file::fileReader::getHead() const {
    return head;
}
void file::fileReader::seek(size_t newHead) {
    if (newHead > fileSize) {
        throw std::out_of_range("Head position is out of file size range.");
    }
    if(newHead < 0) {
        throw std::out_of_range("Head position cannot be negative.");
    }
    head = newHead;
}
file::fileReader::fileReader(const std::string& filePath) : filePath(filePath), head(0), stream(filePath, std::ios::binary| std::ios::in | std::ios::out) {
    if (!stream.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    findEnd();
}
file::fileReader::~fileReader() {
    stream.close();
}

file::fileReaderWriter::fileReaderWriter(const std::string& filePath) : fileReader(filePath) {
}
bool file::fileReader::LoadBlock(long long block) {
    if (block == LoadedBlock) {
        return true; // Block is already loaded
    }
    stream.seekg(block * config::BLOCK_SIZE, std::ios::beg);
    if (!stream.read(buffer, config::BLOCK_SIZE)) {
        return false; // Failed to read the block
    }
    LoadedBlock = block;
    return true;
}
long long file::fileReader::read(char* outBuffer, long long size) {
    long long bytesRead = 0;
    while (bytesRead < size) {
        long long block = head / config::BLOCK_SIZE;
        long long offset = head % config::BLOCK_SIZE;
        if (!LoadBlock(block)) {
            break; // Failed to load the block
        }
        long long bytesToCopy = std::min(size - bytesRead, static_cast<long long>(config::BLOCK_SIZE - offset));
        std::memcpy(outBuffer + bytesRead, buffer + offset, bytesToCopy);
        bytesRead += bytesToCopy;
        this->seek(head + bytesToCopy);
    }
    return bytesRead;
}
long long file::fileReader::read(char* outBuffer, long long size, long long offset) {
    long long bytesRead = 0;
    this->seek(offset);
    return read(outBuffer, size);
}
size_t file::fileReader::getFileSize() const {
    return fileSize;
}
void file::fileReader::findEnd() {
    if (!std::filesystem::exists(filePath)) {
        fileSize = 0;
        head = 0;
        return;
    }

    const uint64_t physicalFileSize =
        std::filesystem::file_size(filePath);

    if (physicalFileSize < config::EndMarkerSize) {
        fileSize = 0;
        head = 0;
        return;
    }    const uint64_t lastBlock =
        (physicalFileSize - 1) / config::BLOCK_SIZE;

    const uint64_t startBlock =
        (lastBlock > 0) ? lastBlock - 1 : 0;

    const uint64_t offset =
        startBlock * config::BLOCK_SIZE;

    const uint64_t bytesToRead = physicalFileSize - offset;

    char temp[config::BLOCK_SIZE * 2];

    stream.clear();
    stream.seekg(offset, std::ios::beg);
    stream.read(temp, bytesToRead);

    const size_t bytesRead = stream.gcount();

    // Not enough data to contain the end marker
    if (bytesRead < static_cast<size_t>(config::EndMarkerSize)) {
        fileSize = 0;
        head = 0;
        return;
    }

    // Search backwards for the last occurrence of EndMarker
    for (size_t i =bytesRead - config::EndMarkerSize;i >= 0; --i) {

        if (std::memcmp(temp + i,config::EndMarker, config::EndMarkerSize) == 0) {
            fileSize = offset + i;
            head = fileSize;
            stream.clear();
            return;
        }
    }
    fileSize = 0;
    head = 0;
    stream.clear();
    return;
}
void file::fileReaderWriter::commit() {
    if (!dirty || LoadedBlock < 0) return;

    stream.clear();
    stream.seekp(static_cast<std::streamoff>(LoadedBlock) * config::BLOCK_SIZE, std::ios::beg);
    stream.write(buffer, config::BLOCK_SIZE);

    if (sizeGrew) {
        size_t markerBlock = fileSize / config::BLOCK_SIZE;

        if (static_cast<size_t>(LoadedBlock) == markerBlock) {
            size_t markerOffsetInBlock = fileSize % config::BLOCK_SIZE;
            size_t spaceInCurrentBlock = config::BLOCK_SIZE - markerOffsetInBlock;

            size_t firstChunk = std::min(spaceInCurrentBlock, static_cast<size_t>(config::EndMarkerSize));
            size_t secondChunk = config::EndMarkerSize - firstChunk;

            // deo 1: ono što stane u trenutni blok
            stream.clear();
            stream.seekp(static_cast<std::streamoff>(fileSize), std::ios::beg);
            stream.write(config::EndMarker, static_cast<std::streamsize>(firstChunk));

            // deo 2: ceo sledeći blok - ostatak markera + zeroed rest
            if (secondChunk > 0) {
                char nextBuf[config::BLOCK_SIZE];
                std::memset(nextBuf, 0, config::BLOCK_SIZE);
                std::memcpy(nextBuf, config::EndMarker + firstChunk, secondChunk);

                stream.clear();
                stream.seekp(static_cast<std::streamoff>(LoadedBlock + 1) * config::BLOCK_SIZE, std::ios::beg);
                stream.write(nextBuf, config::BLOCK_SIZE);
            }

            sizeGrew = false;
        }
    }

    stream.flush();
    dirty = false;
}
bool file::fileReaderWriter::LoadBlock(long long block) {
    if (block == LoadedBlock) {
        return true; // Block is already loaded
    }
    this->commit();
    return fileReader::LoadBlock(block);
}
long long file::fileReaderWriter::write(const char* data, long long size) {
    long long written = 0;

    while (written < size) {
        int block = static_cast<int>(head / config::BLOCK_SIZE);
        long long offsetInBlock = static_cast<long long>(head % config::BLOCK_SIZE);

        if (!LoadBlock(block)) {
            // blok ne postoji na disku (pišemo iza trenutnog fizičkog kraja) -> prazan blok
            std::memset(buffer, 0, config::BLOCK_SIZE);
            LoadedBlock = block;
        }

        long long chunk = std::min<long long>(config::BLOCK_SIZE - offsetInBlock, size - written);
        std::memcpy(buffer + offsetInBlock, data + written, chunk);
        dirty = true;

        head += chunk;
        written += chunk;

        if (head > fileSize) {
            fileSize = head;
            sizeGrew = true;
        }

        if (offsetInBlock + chunk == static_cast<int>(config::BLOCK_SIZE)) {
            commit(); // blok upravo popunjen do kraja -> odmah flush
        }
    }

    return written;
}
void file::fileReader::move(size_t offset) {
    size_t newHead = head + offset;
    seek(newHead);
}
void file::fileReader::ToEnd() {
    seek(fileSize);
}
void file::fileReader::ToStart() {
    seek(0);
}
file::fileReaderWriter::~fileReaderWriter() {
    commit();
}
long long file::fileReader::operator>>(int& data) {
    return read(reinterpret_cast<char*>(&data), sizeof(int));
}
long long file::fileReader::operator>>(char* buffer) {
    return read(buffer, std::strlen(buffer));
}
long long file::fileReaderWriter::operator<<(int data) {
    return write(reinterpret_cast<const char*>(&data), sizeof(long long));
}
long long file::fileReaderWriter::operator<<(double data) {
    return write(reinterpret_cast<const char*>(&data), sizeof(double));
}
long long file::fileReaderWriter::operator<<(const char* buffer) {
    return write(buffer, std::strlen(buffer));
}
long long file::fileReaderWriter::operator<<(const std::string& buffer) {
    return write(buffer.c_str(), buffer.size());
}
long long file::fileReaderWriter::operator<<(long long data) {
    return write(reinterpret_cast<const char*>(&data), sizeof(long long));
} 
long long file::fileReader::operator>>(long long& data) {
    return read(reinterpret_cast<char*>(&data), sizeof(long long));
}
long long file::fileReader::operator>>(double& data) {
    return read(reinterpret_cast<char*>(&data), sizeof(double));
}
long long file::fileReader::operator>>(size_t& data) {
    return read(reinterpret_cast<char*>(&data), sizeof(size_t));
}
long long file::fileReaderWriter::operator<<(size_t data) {
    return write(reinterpret_cast<const char*>(&data), sizeof(size_t));
}
     