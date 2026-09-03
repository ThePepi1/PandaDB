#include <gtest/gtest.h>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

#include "../includes/FileMenager.h"
#include "../includes/fileHendler.h"
#include "../../Utils/config/Block/Block.h"

namespace fs = std::filesystem;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

// Returns a fresh, never-before-used temp file path so tests don't collide
// with each other (FileMenager state is keyed by path and is a singleton,
// so reusing a path across tests could leak state between them).
fs::path makeTempPath(const std::string& tag) {
    static std::atomic<int> counter{0};
    return fs::temp_directory_path() /
           ("filehandler_test_" + tag + "_" + std::to_string(counter++) + ".bin");
}

void createEmptyFile(const fs::path& path) {
    std::ofstream ofs(path, std::ios::binary);
}

void removeIfExists(const fs::path& path) {
    std::error_code ec;
    fs::remove(path, ec);
}

// Builds a deterministic, easy-to-verify byte pattern.
std::vector<char> makePattern(size_t size, char seed = 0) {
    std::vector<char> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<char>((seed + i) % 251);
    }
    return data;
}

constexpr size_t kBlockSize = static_cast<size_t>(config::BLOCK_SIZE);

} 
class FileHandlerTest : public ::testing::Test {
protected:
    fs::path path;

    void SetUp() override {
        path = makeTempPath("basic");
        createEmptyFile(path);
    }

    void TearDown() override {
        removeIfExists(path);
    }
};

TEST_F(FileHandlerTest, GetFileReaderThrowsWhenFileMissing) {
    fs::path missing = makeTempPath("missing");
    removeIfExists(missing);
    EXPECT_THROW(
        file::FileMenager::getInstance().getFileReader(missing.string()),
        std::runtime_error);
}

TEST_F(FileHandlerTest, GetFileReaderWriterThrowsWhenFileMissing) {
    fs::path missing = makeTempPath("missing_rw");
    removeIfExists(missing);
    EXPECT_THROW(
        file::FileMenager::getInstance().getFileReaderWriter(missing.string()),
        std::runtime_error);
}

TEST_F(FileHandlerTest, NewEmptyFileHasZeroSize) {
    auto reader = file::FileMenager::getInstance().getFileReader(path.string());
    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->getFileSize(), 0u);
    EXPECT_EQ(reader->getHead(), 0u);
}

TEST_F(FileHandlerTest, WriteThenCommitPersistsData) {
    auto data = makePattern(64, 1);
    {
        auto writer = file::FileMenager::getInstance().getFileReaderWriter(path.string());
        ASSERT_NE(writer, nullptr);
        writer->write(data.data(), static_cast<int>(data.size()));
        writer->commit(); // explicit commit; see FlushOnDestructionIsNotGuaranteed below
    }

    auto reader = file::FileMenager::getInstance().getFileReader(path.string());
    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->getFileSize(), data.size());

    std::vector<char> readBack(data.size());
    reader->ToStart();
    int bytesRead = reader->read(readBack.data(), static_cast<int>(readBack.size()));
    EXPECT_EQ(bytesRead, static_cast<int>(data.size()));
    EXPECT_EQ(readBack, data);
}

TEST_F(FileHandlerTest, ReadWithOffsetOverloadSeeksFirst) {
    auto data = makePattern(200, 5);
    {
        auto writer = file::FileMenager::getInstance().getFileReaderWriter(path.string());
        writer->write(data.data(), static_cast<int>(data.size()));
        writer->commit();
    }

    auto reader = file::FileMenager::getInstance().getFileReader(path.string());
    std::vector<char> chunk(50);
    int bytesRead = reader->read(chunk.data(), static_cast<int>(chunk.size()), 100);
    EXPECT_EQ(bytesRead, 50);
    EXPECT_EQ(reader->getHead(), 150u);
    EXPECT_TRUE(std::equal(chunk.begin(), chunk.end(), data.begin() + 100));
}

TEST_F(FileHandlerTest, ReadSpansMultipleBlocks) {
    // Enough data to force the read loop to cross a block boundary at least twice.
    auto data = makePattern(kBlockSize * 2 + 137, 9);
    {
        auto writer = file::FileMenager::getInstance().getFileReaderWriter(path.string());
        writer->write(data.data(), static_cast<int>(data.size()));
        writer->commit();
    }

    auto reader = file::FileMenager::getInstance().getFileReader(path.string());
    ASSERT_EQ(reader->getFileSize(), data.size());

    std::vector<char> readBack(data.size());
    reader->ToStart();
    int bytesRead = reader->read(readBack.data(), static_cast<int>(readBack.size()));
    EXPECT_EQ(bytesRead, static_cast<int>(data.size()));
    EXPECT_EQ(readBack, data);
}

TEST_F(FileHandlerTest, SeekBeyondFileSizeThrows) {
    auto data = makePattern(32, 2);
    {
        auto writer = file::FileMenager::getInstance().getFileReaderWriter(path.string());
        writer->write(data.data(), static_cast<int>(data.size()));
        writer->commit();
    }

    auto reader = file::FileMenager::getInstance().getFileReader(path.string());
    EXPECT_THROW(reader->seek(reader->getFileSize() + 1), std::out_of_range);
}

TEST_F(FileHandlerTest, ToStartToEndAndMove) {
    auto data = makePattern(100, 3);
    {
        auto writer = file::FileMenager::getInstance().getFileReaderWriter(path.string());
        writer->write(data.data(), static_cast<int>(data.size()));
        writer->commit();
    }

    auto reader = file::FileMenager::getInstance().getFileReader(path.string());
    reader->ToEnd();
    EXPECT_EQ(reader->getHead(), data.size());

    reader->ToStart();
    EXPECT_EQ(reader->getHead(), 0u);

    reader->move(10);
    EXPECT_EQ(reader->getHead(), 10u);
}

TEST_F(FileHandlerTest, GrowingFileAcrossBlockBoundaryUpdatesSizeAndMarker) {
    // Write exactly across a block boundary so the end-marker relocation
    // logic in commit() (firstChunk/secondChunk split) gets exercised.
    auto data = makePattern(kBlockSize + 10, 7);
    {
        auto writer = file::FileMenager::getInstance().getFileReaderWriter(path.string());
        writer->write(data.data(), static_cast<int>(data.size()));
        writer->commit();
    }

    // Re-open fresh (forces findEnd() to re-derive size from the on-disk marker).
    auto reader = file::FileMenager::getInstance().getFileReader(path.string());
    EXPECT_EQ(reader->getFileSize(), data.size());

    std::vector<char> readBack(data.size());
    reader->ToStart();
    reader->read(readBack.data(), static_cast<int>(readBack.size()));
    EXPECT_EQ(readBack, data);
}

TEST_F(FileHandlerTest, AppendingInMultipleWritesAccumulates) {
    auto first = makePattern(40, 1);
    auto second = makePattern(60, 2);
    {
        auto writer = file::FileMenager::getInstance().getFileReaderWriter(path.string());
        writer->write(first.data(), static_cast<int>(first.size()));
        writer->write(second.data(), static_cast<int>(second.size()));
        writer->commit();
    }

    auto reader = file::FileMenager::getInstance().getFileReader(path.string());
    EXPECT_EQ(reader->getFileSize(), first.size() + second.size());

    std::vector<char> readBack(first.size() + second.size());
    reader->ToStart();
    reader->read(readBack.data(), static_cast<int>(readBack.size()));

    std::vector<char> expected;
    expected.insert(expected.end(), first.begin(), first.end());
    expected.insert(expected.end(), second.begin(), second.end());
    EXPECT_EQ(readBack, expected);
}

TEST_F(FileHandlerTest, FlushOnDestructionIsNotGuaranteed) {
    auto data = makePattern(20, 4); // smaller than one block -> never auto-committed
    {
        auto writer = file::FileMenager::getInstance().getFileReaderWriter(path.string());
        writer->write(data.data(), static_cast<int>(data.size()));
        // intentionally no commit() call; writer goes out of scope here
    }

    auto reader = file::FileMenager::getInstance().getFileReader(path.string());

    EXPECT_EQ(reader->getFileSize(), 20u);
}

// ---------------------------------------------------------------------------
// FileMenager: singleton identity
// ---------------------------------------------------------------------------
TEST(FileMenagerSingletonTest, GetInstanceReturnsSameObject) {
    auto& a = file::FileMenager::getInstance();
    auto& b = file::FileMenager::getInstance();
    EXPECT_EQ(&a, &b);
}

class FileMenagerLockingTest : public ::testing::Test {
protected:
    fs::path path;

    void SetUp() override {
        path = makeTempPath("locking");
        createEmptyFile(path);
    }

    void TearDown() override {
        removeIfExists(path);
    }
};

TEST_F(FileMenagerLockingTest, MultipleConcurrentReadersAllowed) {
    auto& mgr = file::FileMenager::getInstance();
    auto r1 = mgr.getFileReader(path.string());
    auto r2 = mgr.getFileReader(path.string());
    EXPECT_NE(r1, nullptr);
    EXPECT_NE(r2, nullptr);
}

TEST_F(FileMenagerLockingTest, GetFileReaderFailsWhileWriterActive) {
    auto& mgr = file::FileMenager::getInstance();
    auto writer = mgr.getFileReaderWriter(path.string());
    ASSERT_NE(writer, nullptr);

    auto reader = mgr.getFileReader(path.string());
    EXPECT_EQ(reader, nullptr);

    writer.reset(); // releases the writer
    auto readerAfter = mgr.getFileReader(path.string());
    EXPECT_NE(readerAfter, nullptr);
}

TEST_F(FileMenagerLockingTest, GetFileReaderWriterFailsWhileReaderActive) {
    auto& mgr = file::FileMenager::getInstance();
    auto reader = mgr.getFileReader(path.string());
    ASSERT_NE(reader, nullptr);

    auto writer = mgr.getFileReaderWriter(path.string());
    EXPECT_EQ(writer, nullptr);

    reader.reset(); // releases the reader
    auto writerAfter = mgr.getFileReaderWriter(path.string());
    EXPECT_NE(writerAfter, nullptr);
}

TEST_F(FileMenagerLockingTest, GetFileReaderWriterFailsWhileWriterActive) {
    auto& mgr = file::FileMenager::getInstance();
    auto writer1 = mgr.getFileReaderWriter(path.string());
    ASSERT_NE(writer1, nullptr);

    auto writer2 = mgr.getFileReaderWriter(path.string());
    EXPECT_EQ(writer2, nullptr);
}

TEST_F(FileMenagerLockingTest, ReleasingReaderDecrementsCountBeforeAllowingWriter) {
    auto& mgr = file::FileMenager::getInstance();
    auto r1 = mgr.getFileReader(path.string());
    auto r2 = mgr.getFileReader(path.string());
    ASSERT_NE(r1, nullptr);
    ASSERT_NE(r2, nullptr);

    r1.reset(); // one reader left
    EXPECT_EQ(mgr.getFileReaderWriter(path.string()), nullptr);

    r2.reset(); // no readers left
    auto writer = mgr.getFileReaderWriter(path.string());
    EXPECT_NE(writer, nullptr);
}

class FileMenagerBlockingTest : public ::testing::Test {
protected:
    fs::path path;

    void SetUp() override {
        path = makeTempPath("blocking");
        createEmptyFile(path);
    }

    void TearDown() override {
        removeIfExists(path);
    }
};

TEST_F(FileMenagerBlockingTest, SubscribeReaderWaitsForWriterToRelease) {
    auto& mgr = file::FileMenager::getInstance();
    auto writer = mgr.getFileReaderWriter(path.string());
    ASSERT_NE(writer, nullptr);

    std::atomic<bool> readerAcquired{false};
    std::thread waiter([&]() {
        auto reader = mgr.subbscribeFileReader(path.string());
        readerAcquired = true;
        EXPECT_NE(reader, nullptr);
    });

    // Give the waiter thread a chance to actually start waiting.
    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(readerAcquired.load())
        << "Reader should still be blocked while the writer holds the file";

    writer.reset(); // release the writer, should wake the waiter

    waiter.join();
    EXPECT_TRUE(readerAcquired.load());
}

TEST_F(FileMenagerBlockingTest, SubscribeReaderWriterWaitsForReadersToRelease) {
    auto& mgr = file::FileMenager::getInstance();
    auto reader = mgr.getFileReader(path.string());
    ASSERT_NE(reader, nullptr);

    std::atomic<bool> writerAcquired{false};
    std::thread waiter([&]() {
        auto rw = mgr.subbscribeFileReaderWriter(path.string());
        writerAcquired = true;
        EXPECT_NE(rw, nullptr);
    });

    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(writerAcquired.load())
        << "Writer should still be blocked while a reader holds the file";

    reader.reset();

    waiter.join();
    EXPECT_TRUE(writerAcquired.load());
}

TEST_F(FileMenagerBlockingTest, MultipleSubscribersEventuallyAllGetAccess) {
    auto& mgr = file::FileMenager::getInstance();
    auto writer = mgr.getFileReaderWriter(path.string());
    ASSERT_NE(writer, nullptr);

    constexpr int kSubscribers = 4;
    std::atomic<int> acquiredCount{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < kSubscribers; ++i) {
        threads.emplace_back([&]() {
            auto reader = mgr.subbscribeFileReader(path.string());
            if (reader) {
                acquiredCount.fetch_add(1);
            }
            // hold briefly then release
            std::this_thread::sleep_for(10ms);
        });
    }

    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(acquiredCount.load(), 0)
        << "No subscriber should have acquired access while writer is active";

    writer.reset();

    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(acquiredCount.load(), kSubscribers);
}