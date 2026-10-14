#pragma once
#include <string>
#include <fstream>
#include <vector>
#include "../../Utils/config/Block/Block.h"
#include <liburing.h>

namespace file {
    class FileMenager;
    class fileReader { 
        protected :
            fileReader(const std::string& filePath) ;
            fileReader() = default;
            fileReader(const std::string& filePath, bool createIfNotExists);
            virtual ~fileReader();  
            size_t head;
            size_t fileSize;
            std::fstream stream;
            char buffer[config::SIZE_OFF_BLOCK];
            long long LoadedBlock = -1;
            std::string filePath;
            virtual bool LoadBlock(long long block);
            
        public:
            size_t getHead() const;
            void seek(size_t newHead);
            long long read(char* buffer, long long size);
            long long read(char* buffer, long long size, long long offset);
            size_t getFileSize() const;
            void ToEnd();
            void ToStart();
            void move(size_t offset);
            void findEnd();
            friend class FileMenager;
            long long operator>>(int& data);
            long long operator>>(char* buffer);
            long long operator>>(long long& data);
            long long operator>>(double& data);
            long long operator>>(size_t& data);
            long long operator>>(std::string& buffer);
            struct Deleter {
                void operator()(fileReader* reader) const;
            };
            void beam(std::vector<char*> &buffer, std::vector<size_t> &positionst, std::vector<size_t> &sizes);
            template<typename T>
            fileReader &operator>>(std::vector<T> &data); 
    };
    class fileReaderWriter : public fileReader { 
        private :
            fileReaderWriter(const std::string& filePath);
            
            bool dirty = false;
            bool sizeGrew = false;
            bool LoadBlock(long long block) override;
        public:
            virtual ~fileReaderWriter() override;
            void commit();
            long long write(const char* buffer, long long size);
            struct Deleter {
                void operator()(fileReaderWriter* reader) const;
            };
            long long operator <<(size_t data);
            long long operator<<(int data);
            long long operator<<(const char* buffer);
            long long operator<<(const std::string& buffer); 
            long long operator<<(long long data);
            long long operator<<(double data);
            template<typename T>
            fileReaderWriter &operator<<(const std::vector<T> &data);


            friend class FileMenager;
            
    };
}