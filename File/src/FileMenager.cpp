#include "../includes/FileMenager.h"
#include "../includes/fileHendler.h"

file::FileMenager& file::FileMenager::getInstance() {
    static file::FileMenager instance;
    return instance;
}

std::unique_ptr<file::fileReader, file::fileReader::Deleter> file::FileMenager::getFileReader(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mtx);
    if (activeWriters.contains(filePath)) {
        return nullptr;
    }
    if (waitingAccess.contains(filePath) && !waitingAccess[filePath].empty()) {
        return nullptr;
    }    
    auto it = std::unique_ptr<fileReader, fileReader::Deleter>(new fileReader(filePath)); // Need to check is there any issues here without using make_unique
    activeReaders[filePath]++;
    return it; // Need to check is there any issues here without using make_unique

}
std::unique_ptr<file::fileReader, file::fileReader::Deleter> file::FileMenager::subbscribeFileReader(const std::string& filePath) {
    std::unique_lock<std::mutex> lock(mtx);
    std::condition_variable cv;
    if(waitingAccess.contains(filePath)) {
            waitingAccess[filePath].push(&cv);
    } else {
            std::queue<std::condition_variable*> newQueue;
            newQueue.push(&cv);
            waitingAccess[filePath] = std::move(newQueue);
    }
    while( activeWriters.contains(filePath) || 
    waitingAccess.contains(filePath) && !waitingAccess[filePath].empty() && waitingAccess[filePath].front() != &cv ) {
        cv.wait(lock);
    }
    waitingAccess[filePath].pop();
    auto it = std::unique_ptr<fileReader, fileReader::Deleter>(new fileReader(filePath)); // Need to check is there any issues here without using make_unique
    activeReaders[filePath]++;
    if(waitingAccess.contains(filePath) && !waitingAccess[filePath].empty()) {
        waitingAccess[filePath].front()->notify_one();
    }
    return it; // Need to check is there any issues here without using make_unique
} 
std::unique_ptr<file::fileReaderWriter, file::fileReaderWriter::Deleter> file::FileMenager::getFileReaderWriter(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mtx);
    if (activeWriters.contains(filePath)) {
        return nullptr;
    }
    if (activeReaders.contains(filePath) && activeReaders[filePath] > 0) {
        return nullptr;
    }
    if (waitingAccess.contains(filePath) && !waitingAccess[filePath].empty()) {
        return nullptr;
    }  
    auto it = std::unique_ptr<fileReaderWriter, fileReaderWriter::Deleter>(new fileReaderWriter(filePath)); // Need to check is there any issues here without using make_unique  
    activeWriters.insert(filePath);
    return it;
}
std::unique_ptr<file::fileReaderWriter, file::fileReaderWriter::Deleter> file::FileMenager::subbscribeFileReaderWriter(const std::string& filePath) {
    std::unique_lock<std::mutex> lock(mtx); 
    std::condition_variable cv;
    if(waitingAccess.contains(filePath)) {
            waitingAccess[filePath].push(&cv);
        } else {
            std::queue<std::condition_variable*> newQueue;
            newQueue.push(&cv);
            waitingAccess[filePath] = std::move(newQueue);
        }
    while(activeWriters.contains(filePath) || activeReaders.contains(filePath) && activeReaders[filePath] > 0 || waitingAccess.contains(filePath) && !waitingAccess[filePath].empty() && waitingAccess[filePath].front() != &cv ) {
        cv.wait(lock);
    }
    auto it = std::unique_ptr<fileReaderWriter, fileReaderWriter::Deleter>(new fileReaderWriter(filePath)); // Need to check is there any issues here without using make_unique
    waitingAccess[filePath].pop();
    activeWriters.insert(filePath);
    if(waitingAccess.contains(filePath) && !waitingAccess[filePath].empty()) {
        waitingAccess[filePath].front()->notify_one();
    }
    return it;
}
void file::FileMenager::releseFileReader(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mtx);
    if (activeReaders.contains(filePath)) {
        activeReaders[filePath]--;
        if (activeReaders[filePath] == 0) {
            activeReaders.erase(filePath);
        }
    }
    if (waitingAccess.contains(filePath) && !waitingAccess[filePath].empty()) {
        waitingAccess[filePath].front()->notify_one();
    }
}
void file::FileMenager::releseFileReaderWriter(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mtx);
    activeWriters.erase(filePath);
    if (waitingAccess.contains(filePath) && !waitingAccess[filePath].empty()) {
        waitingAccess[filePath].front()->notify_one();
    }
}
