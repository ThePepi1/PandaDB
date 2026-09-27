#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <cstdio>
#include <vector>
#include <filesystem>
#include "Vector/FactoryVector.h"
#include "Vector/FactoryVectorCPU.h"
#include "../includes/linearStorage.h"
#include "../../Metrics/includes/MetricFactory.h"
#include "../../Data/VectorStream/includes/VectorStreams.h"
#include "../../Metrics/includes/IMetrics.h"
#include "../../Metrics/includes/register.h"

using namespace std;
using namespace data;
using namespace metrics;
using namespace DataStructures;

static FactoryVector *ls_factory_vector = new FactoryVectorCPU();

class LinearStorageTest : public ::testing::Test {
protected:
    std::string filePath;

    void SetUp() override {
        register_builtin_metrics();
        const ::testing::TestInfo* info = ::testing::UnitTest::GetInstance()->current_test_info();
        filePath = std::string("ls_test_") + info->test_suite_name() + "_" + info->name() + ".bin";
        std::remove(filePath.c_str()); // in case a previous crashed run left it behind
    }

    void TearDown() override {
        std::remove(filePath.c_str()); // clean up whatever this test created
    }

    // Two vectors are "equal" if euclidean distance between them is ~0.
    bool vectorsEqual(const IVector& a, const IVector& b, double tol = 1e-9) {
        auto metric = MetricFactory::instance().create_metric("euclide");
        return (*metric)(a, b) <= tol;
    }
};

TEST_F(LinearStorageTest, ConstructorWritesHeader) {
    DataStructures::LinearVectorStorage storage(filePath, 8, 0);
    EXPECT_EQ(storage.getDim(), 8u);
    EXPECT_EQ(storage.getSize(), 0);
    EXPECT_EQ(storage.getFilePath(), filePath);
}

TEST_F(LinearStorageTest, ReopenExistingFileReadsHeaderBack) {
    {
        LinearVectorStorage storage(filePath, 5, 0);
    }
    LinearVectorStorage reopened(filePath);
    EXPECT_EQ(reopened.getDim(), 5u);
    EXPECT_EQ(reopened.getSize(), 0);
}

TEST_F(LinearStorageTest, SaveVectorIncrementsSize) {
    LinearVectorStorage storage(filePath, 4, 0);
    auto v = ls_factory_vector->createVector(4, 2.5);
    storage.saveVector(*v);
    EXPECT_EQ(storage.getSize(), 1);
    storage.saveVector(*v);
    EXPECT_EQ(storage.getSize(), 2);
}


TEST_F(LinearStorageTest, SaveThenLoadSingleVectorMatches) {
    LinearVectorStorage storage(filePath, 6, 0);
    auto original = ls_factory_vector->createVector(6, 3.75);
    storage.saveVector(*original);

    auto loaded = ls_factory_vector->createVector(6);
    storage.loadVector(0, *loaded);
    EXPECT_TRUE(vectorsEqual(*original, *loaded));
}

TEST_F(LinearStorageTest, SaveThenLoadMultipleVectorsMatchByIndex) {
    LinearVectorStorage storage(filePath, 10, 0);
    std::vector<double> fills = {0.0, 1.5, 3.25, -2.0, 100.0};
    for (double f : fills) {
        auto v = ls_factory_vector->createVector(10, f);
        storage.saveVector(*v);
    }
    ASSERT_EQ(storage.getSize(), static_cast<long long>(fills.size()));

    for (size_t i = 0; i < fills.size(); ++i) {
        auto expected = ls_factory_vector->createVector(10, fills[i]);
        auto loaded = ls_factory_vector->createVector(10);
        storage.loadVector(static_cast<long long>(i), *loaded);
        EXPECT_TRUE(vectorsEqual(*expected, *loaded)) << "mismatch at index " << i;
    }
}

TEST_F(LinearStorageTest, LoadVectorOutOfRangeThrows) {
    LinearVectorStorage storage(filePath, 4, 0);
    auto v = ls_factory_vector->createVector(4, 1.0);
    storage.saveVector(*v);

    auto placeholder = ls_factory_vector->createVector(4);
    EXPECT_THROW(storage.loadVector(-1, *placeholder), std::out_of_range);
    EXPECT_THROW(storage.loadVector(1, *placeholder), std::out_of_range); // only index 0 exists
}

TEST_F(LinearStorageTest, VectorStreamFullRangeReturnsAllVectors) {
    LinearVectorStorage storage(filePath, 3, 0);
    const int n = 5;
    for (int i = 0; i < n; ++i) {
        auto v = ls_factory_vector->createVector(3, static_cast<double>(i));
        storage.saveVector(*v);
    }

    auto stream = storage.getVectorStream(*ls_factory_vector);
    ASSERT_NE(stream, nullptr);
    EXPECT_EQ(stream->getDim(), 3);

    int count = 0;
    while (stream->hasNext()) {
        auto v = stream->next();
        ASSERT_NE(v, nullptr);
        ++count;
    }
    EXPECT_EQ(count, n);
    EXPECT_FALSE(stream->hasNext());
    EXPECT_THROW(stream->next(), std::out_of_range);
}

TEST_F(LinearStorageTest, VectorStreamGetVectorsBatch) {
    LinearVectorStorage storage (filePath, 2, 0);
    const int n = 4;
    for (int i = 0; i < n; ++i) {
        auto v = ls_factory_vector->createVector(2, static_cast<double>(i));
        storage.saveVector(*v);
    }

    auto stream = storage.getVectorStream(*ls_factory_vector);
    auto batch = stream->getVectors(2);
    EXPECT_EQ(batch.size(), 2u);
    auto rest = stream->getVectors(10); // asks for more than remains
    EXPECT_EQ(rest.size(), 2u);
    EXPECT_FALSE(stream->hasNext());
}

TEST_F(LinearStorageTest, VectorStreamRangeThrowsOnInvalidBounds) {
    LinearVectorStorage storage(filePath, 2, 0);
    for (int i = 0; i < 3; ++i) {
        auto v = ls_factory_vector->createVector(2, static_cast<double>(i));
        storage.saveVector(*v);
    }
    EXPECT_THROW(storage.getVectorStream(2, 1, *ls_factory_vector), std::out_of_range);  // start >= end
    EXPECT_THROW(storage.getVectorStream(0, 10, *ls_factory_vector), std::out_of_range); // end > size
}

TEST_F(LinearStorageTest, VectorStreamRangeReturnsRequestedCount) {
    LinearVectorStorage storage(filePath, 2, 0);
    const int n = 6;
    for (int i = 0; i < n; ++i) {
        auto v = ls_factory_vector->createVector(2, static_cast<double>(i));
        storage.saveVector(*v);
    }

    auto stream = storage.getVectorStream(2, 5, *ls_factory_vector); // expect 3 vectors
    ASSERT_NE(stream, nullptr);
    int count = 0;
    while (stream->hasNext()) {
        stream->next();
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(LinearStorageTest, VectorStreamResetReturnsToStart) {
    LinearVectorStorage storage(filePath, 2, 0);
    for (int i = 0; i < 4; ++i) {
        auto v = ls_factory_vector->createVector(2, static_cast<double>(i));
        storage.saveVector(*v);
    }
    auto stream = storage.getVectorStream(1, 4, *ls_factory_vector);
    while (stream->hasNext()) stream->next();
    EXPECT_FALSE(stream->hasNext());

    stream->reset();
    EXPECT_TRUE(stream->hasNext());
}

