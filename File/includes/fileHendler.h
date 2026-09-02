#pragma once
#include <string>
#include <fstream>
namespace file {
    class FileMenager;
    class fileReader { 
        protected :
            fileReader(const std::string& filePath) ;
            fileReader() = default;
            ~fileReader();  
            size_t head;
            std::ifstream stream;
            char buffer[1024];
            int LoadedBlock = -1;
            std::string filePath;
            bool LoadBlock(int block);
            
        public:
            size_t getHead() const;
            void setHead(size_t newHead);
            int read(char* buffer, int size);
            int read(char* buffer, int size, int offset);
            friend class FileMenager;
            struct Deleter {
                void operator()(fileReader* reader) const;
            };
    };
    class fileReaderWriter : public fileReader { 
        private :
            fileReaderWriter(const std::string& filePath);
        public:
            struct Deleter {
                void operator()(fileReaderWriter* reader) const;
            };

    };
}