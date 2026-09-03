#pragma once
#include <string>
#include <fstream>
#include "../../Utils/config/Block/Block.h"

namespace file {
    class FileMenager;
    class fileReader { 
        protected :
            fileReader(const std::string& filePath) ;
            fileReader() = default;
            virtual ~fileReader();  
            size_t head;
            size_t fileSize;
            std::fstream stream;
            char buffer[config::BLOCK_SIZE];
            int LoadedBlock = -1;
            std::string filePath;
            virtual bool LoadBlock(int block);
            
        public:
            size_t getHead() const;
            void seek(size_t newHead);
            int read(char* buffer, int size);
            int read(char* buffer, int size, int offset);
            size_t getFileSize() const;
            void ToEnd();
            void ToStart();
            void move(size_t offset);
            void findEnd();
            friend class FileMenager;
            struct Deleter {
                void operator()(fileReader* reader) const;
            };
    };
    class fileReaderWriter : public fileReader { 
        private :
            fileReaderWriter(const std::string& filePath);
            bool dirty = false;
            bool sizeGrew = false;
            bool LoadBlock(int block) override;
        public:
            virtual ~fileReaderWriter() override;
            void commit();
            int write(const char* buffer, int size);
            struct Deleter {
                void operator()(fileReaderWriter* reader) const;
            };
            friend class FileMenager;
            
    };
}