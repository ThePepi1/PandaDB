#include "../includes/fileHendler.h"
#include "../includes/FileMenager.h"
#include "../../Utils/config/BlockSize/BlockSize.h"


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
void file::fileReader::setHead(size_t newHead) {
    head = newHead;
}
file::fileReader::fileReader(const std::string& filePath) : filePath(filePath), head(0), stream(filePath, std::ios::binary| std::ios::in | std::ios::out) {
    if (!stream.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
}
file::fileReader::~fileReader() {\
    stream.close();
}

file::fileReaderWriter::fileReaderWriter(const std::string& filePath) : fileReader(filePath) {
}

bool file::fileReader::LoadBlock(int block) {
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

