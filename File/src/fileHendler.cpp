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

file::fileReaderWriter::fileReaderWriter(const std::string& filePath) : fileReader(filePath, true) {
}
bool file::fileReader::LoadBlock(long long block) {
    if (block == LoadedBlock) {
        return true; // Block is already loaded
    }
    stream.seekg(block * config::SIZE_OFF_BLOCK, std::ios::beg);
    if (!stream.read(buffer, config::SIZE_OFF_BLOCK)) {
        return false; // Failed to read the block
    }
    LoadedBlock = block;
    return true;
}
long long file::fileReader::read(char* outBuffer, long long size) {
    long long bytesRead = 0;
    while (bytesRead < size) {
        long long block = head / config::SIZE_OFF_BLOCK;
        long long offset = head % config::SIZE_OFF_BLOCK;
        if (!LoadBlock(block)) {
            break; // Failed to load the block
        }
        long long bytesToCopy = std::min(size - bytesRead, static_cast<long long>(config::SIZE_OFF_BLOCK - offset));
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
        (physicalFileSize - 1) / config::SIZE_OFF_BLOCK;

    const uint64_t startBlock =
        (lastBlock > 0) ? lastBlock - 1 : 0;

    const uint64_t offset =
        startBlock * config::SIZE_OFF_BLOCK;

    const uint64_t bytesToRead = physicalFileSize - offset;

    char temp[config::SIZE_OFF_BLOCK * 2];

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
    for (long long i =bytesRead - config::EndMarkerSize;i >= 0; --i) {

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
    stream.seekp(static_cast<std::streamoff>(LoadedBlock) * config::SIZE_OFF_BLOCK, std::ios::beg);
    stream.write(buffer, config::SIZE_OFF_BLOCK);

    if (sizeGrew) {
        size_t markerBlock = fileSize / config::SIZE_OFF_BLOCK;

        if (static_cast<size_t>(LoadedBlock) == markerBlock) {
            size_t markerOffsetInBlock = fileSize % config::SIZE_OFF_BLOCK;
            size_t spaceInCurrentBlock = config::SIZE_OFF_BLOCK - markerOffsetInBlock;

            size_t firstChunk = std::min(spaceInCurrentBlock, static_cast<size_t>(config::EndMarkerSize));
            size_t secondChunk = config::EndMarkerSize - firstChunk;

            // deo 1: ono što stane u trenutni blok
            stream.clear();
            stream.seekp(static_cast<std::streamoff>(fileSize), std::ios::beg);
            stream.write(config::EndMarker, static_cast<std::streamsize>(firstChunk));

            // deo 2: ceo sledeći blok - ostatak markera + zeroed rest
            if (secondChunk > 0) {
                char nextBuf[config::SIZE_OFF_BLOCK];
                std::memset(nextBuf, 0, config::SIZE_OFF_BLOCK);
                std::memcpy(nextBuf, config::EndMarker + firstChunk, secondChunk);

                stream.clear();
                stream.seekp(static_cast<std::streamoff>(LoadedBlock + 1) * config::SIZE_OFF_BLOCK, std::ios::beg);
                stream.write(nextBuf, config::SIZE_OFF_BLOCK);
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
        int block = static_cast<int>(head / config::SIZE_OFF_BLOCK);
        long long offsetInBlock = static_cast<long long>(head % config::SIZE_OFF_BLOCK);

        if (!LoadBlock(block)) {
            // blok ne postoji na disku (pišemo iza trenutnog fizičkog kraja) -> prazan blok
            std::memset(buffer, 0, config::SIZE_OFF_BLOCK);
            LoadedBlock = block;
        }

        long long chunk = std::min<long long>(config::SIZE_OFF_BLOCK - offsetInBlock, size - written);
        std::memcpy(buffer + offsetInBlock, data + written, chunk);
        dirty = true;

        head += chunk;
        written += chunk;

        if (head > fileSize) {
            fileSize = head;
            sizeGrew = true;
        }

        if (offsetInBlock + chunk == static_cast<int>(config::SIZE_OFF_BLOCK)) {
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
    return write(reinterpret_cast<const char*>(&data), sizeof(int));
}
long long file::fileReaderWriter::operator<<(double data) {
    return write(reinterpret_cast<const char*>(&data), sizeof(double));
}
long long file::fileReaderWriter::operator<<(const char* buffer) {
    return write(buffer, std::strlen(buffer));
}
long long file::fileReaderWriter::operator<<(const std::string& buffer) {
    size_t size = buffer.size();
    size = this->operator<<(size); // Write the size of the string first
    return write(buffer.c_str(), buffer.size()) + size;
}
long long file::fileReader::operator>>(std::string& buffer) {
    size_t size;
    long long bytesRead = this->operator>>(size); // Read the size of the string first
    if (bytesRead <= 0) {
        return bytesRead; // Return if reading size failed
    }
    buffer.resize(size);
    long long readBytes = read(&buffer[0], size); // Read the actual string data
    return bytesRead + readBytes; // Return total bytes read (size + string data)
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
file::fileReader::fileReader(
    const std::string& filePath,bool createIfMissing)
    : filePath(filePath),
      head(0),
      stream(){
    if (createIfMissing && !std::filesystem::exists(filePath)) {
        std::ofstream create(filePath, std::ios::binary);
        if (!create) {
            throw std::runtime_error(
                "Failed to create file: " + filePath
            );
        }
    }

    stream.open(
        filePath,
        std::ios::binary |
        std::ios::in |
        std::ios::out
    );

    if (!stream.is_open()) {
        throw std::runtime_error(
            "Failed to open file: " + filePath
        );
    }

    findEnd();
}   
void file::fileReader::beam(std::vector<char*> &buffer, std::vector<size_t> &positionst, std::vector<size_t> &sizes) {
    if (buffer.size() != positionst.size() || buffer.size() != sizes.size() || positionst.empty()) {
        throw std::invalid_argument("Buffer, positions, and sizes vectors must have the same size.");
    }
#ifdef __linux__
    constexpr size_t ALIGN = 4096;
    const size_t count = buffer.size();

    // ---- fd (RAII) ----
    struct Fd {
        int v;
        ~Fd() { if (v >= 0) ::close(v); }
    } fd{ ::open(filePath.c_str(), O_RDONLY | O_DIRECT) };
    if (fd.v < 0) {
        fd.v = ::open(filePath.c_str(), O_RDONLY);
        if (fd.v < 0) throw std::runtime_error("Failed to open file for io_uring: " + filePath);
    }

    // ---- ring (RAII) ----
    struct Ring {
        io_uring r;
        bool ok = false;
        ~Ring() { if (ok) io_uring_queue_exit(&r); }
    } ring;
    if (int rc = io_uring_queue_init(static_cast<unsigned>(count), &ring.r, 0); rc < 0) {
        throw std::runtime_error("Failed to initialize io_uring: " + std::to_string(rc));
    }
    ring.ok = true;

    // ---- poravnati privremeni baferi (O_DIRECT trazi poravnat offset, duzinu i adresu) ----
    struct Req {
        std::unique_ptr<char, void (*)(void*)> raw{nullptr, std::free};
        size_t skip = 0;   // koliko bajtova od pocetka poravnatog bloka treba preskociti
    };
    std::vector<Req> reqs(count);

    for (size_t i = 0; i < count; ++i) {
        const size_t aPos = positionst[i] & ~(ALIGN - 1);
        const size_t aEnd = (positionst[i] + sizes[i] + ALIGN - 1) & ~(ALIGN - 1);
        reqs[i].skip = positionst[i] - aPos;
        reqs[i].raw.reset(static_cast<char*>(std::aligned_alloc(ALIGN, aEnd - aPos)));
        if (!reqs[i].raw) throw std::bad_alloc();

        io_uring_sqe* sqe = io_uring_get_sqe(&ring.r);
        if (!sqe) throw std::runtime_error("Failed to get submission queue entry for io_uring.");
        io_uring_prep_read(sqe, fd.v, reqs[i].raw.get(), static_cast<unsigned>(aEnd - aPos), aPos);
        io_uring_sqe_set_data64(sqe, i);
    }

    // ---- submit + wait: SVE kompletirano pre nego sto baferi izadju iz scope-a ----
    int submitted = io_uring_submit_and_wait(&ring.r, static_cast<unsigned>(count));
    if (submitted < 0) {
        throw std::runtime_error("Failed to submit io_uring operations: " + std::to_string(submitted));
    }

    // ---- completions: kopiraj trazene bajtove u izlazne bafere ----
    io_uring_cqe* cqe;
    unsigned head;
    unsigned completed = 0;
    int errorCode = 0;

    io_uring_for_each_cqe(&ring.r, head, cqe) {
        ++completed;
        const size_t i = static_cast<size_t>(io_uring_cqe_get_data64(cqe));
        if (cqe->res < 0) {
            if (!errorCode) errorCode = cqe->res;
        } else if (static_cast<size_t>(cqe->res) < reqs[i].skip + sizes[i]) {
            if (!errorCode) errorCode = -EIO;          // kratko citanje (iza kraja fajla)
        } else {
            std::memcpy(buffer[i], reqs[i].raw.get() + reqs[i].skip, sizes[i]);
        }
    }
    io_uring_cq_advance(&ring.r, completed);

    if (errorCode) {
        throw std::runtime_error("io_uring read operation failed with kernel error code: " + std::to_string(errorCode));
    }
#else
    for (size_t i = 0; i < buffer.size(); ++i) {
        read(buffer[i], static_cast<long long>(sizes[i]), static_cast<long long>(positionst[i]));
    }
#endif
}
template<typename T>
file::fileReader & operator>>(file::fileReader& reader, std::vector<T> &data) {
    size_t size;
    reader >> size;
    data.resize(size);
    for (size_t i = 0; i < size; ++i) {
        reader >> data[i];
    }
    return reader;
}
template<typename T>
file::fileReaderWriter & operator<<(file::fileReaderWriter& writer, const std::vector<T> &data) {
    size_t size = data.size();
    writer << size;
    for (const auto& item : data) {
        writer << item;
    }
    return writer;
}