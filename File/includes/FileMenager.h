#pragma once
#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <condition_variable>
#include "fileHendler.h"

namespace file {

    class FileMenager { 
        private :
            FileMenager() = default;
            FileMenager(const FileMenager&) = delete;
            FileMenager& operator=(const FileMenager&) = delete;
            FileMenager(FileMenager&&) = delete;
            FileMenager& operator=(FileMenager&&) = delete;
            std::mutex mtx;
            std::unordered_set<std::string> activeWriters;
            std::unordered_map<std::string, int> activeReaders;
            std::unordered_map<std::string, std::queue<std::condition_variable*>> waitingAccess;
            void releseFileReader(const std::string& filePath);
            void releseFileReaderWriter(const std::string& filePath);

        public:
            std::unique_ptr<fileReader , fileReader::Deleter > getFileReader(const std::string& filePath);
            std::unique_ptr<fileReaderWriter, fileReaderWriter::Deleter> getFileReaderWriter(const std::string& filePath);
            std::unique_ptr<fileReader, fileReader::Deleter> subbscribeFileReader(const std::string& filePath);
            std::unique_ptr<fileReaderWriter, fileReaderWriter::Deleter> subbscribeFileReaderWriter(const std::string& filePath);
            static FileMenager& getInstance() ;
            friend class fileReader;
            friend class fileReaderWriter;
    };

}