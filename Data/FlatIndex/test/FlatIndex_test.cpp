// Data/FlatIndex/test/FlatIndex_test.cpp

#include <gtest/gtest.h>
#include "../includes/FlatenIndex.h"
#include "../../../MathCompponents/includes/Vector/FactoryVectorCPU.h"
#include "../../../MathCompponents/includes/Vector/IVector.h"
#include <cstdint>
#include <stdexcept>
#include <memory>

using Data::FlatenIndex;
using data::FactoryVectorCPU;
using data::IVector;

static FactoryVectorCPU g_factory;


TEST(FlatenIndexConstruction, StoresElementCount) {
    FlatenIndex fi(10, 4, &g_factory);
    EXPECT_EQ(fi.elements(), 10u);
}

TEST(FlatenIndexConstruction, StoresDimension) {
    FlatenIndex fi(10, 4, &g_factory);
    EXPECT_EQ(fi.dimension(), 4u);
}

TEST(FlatenIndexConstruction, NullFactoryDoesNotThrow) {
    EXPECT_NO_THROW(FlatenIndex(4, 4, nullptr));
}



TEST(FlatenIndexPadding, AlreadyAligned_4)  { EXPECT_EQ(FlatenIndex(1,4,&g_factory).padding(), 4u);   }
TEST(FlatenIndexPadding, AlreadyAligned_8)  { EXPECT_EQ(FlatenIndex(1,8,&g_factory).padding(), 8u);   }
TEST(FlatenIndexPadding, NeedsOnePad)       { EXPECT_EQ(FlatenIndex(1,5,&g_factory).padding(), 8u);   }
TEST(FlatenIndexPadding, NeedsTwoPad)       { EXPECT_EQ(FlatenIndex(1,6,&g_factory).padding(), 8u);   }
TEST(FlatenIndexPadding, NeedsThreePad)     { EXPECT_EQ(FlatenIndex(1,7,&g_factory).padding(), 8u);   }
TEST(FlatenIndexPadding, DimensionOne)      { EXPECT_EQ(FlatenIndex(1,1,&g_factory).padding(), 4u);   }
TEST(FlatenIndexPadding, DimensionThree)    { EXPECT_EQ(FlatenIndex(1,3,&g_factory).padding(), 4u);   }
TEST(FlatenIndexPadding, LargeAligned)      { EXPECT_EQ(FlatenIndex(1,100,&g_factory).padding(),100u);}
TEST(FlatenIndexPadding, LargeNonAligned)   { EXPECT_EQ(FlatenIndex(1,101,&g_factory).padding(),104u);}


TEST(FlatenIndexAlignment, RowZeroIs32ByteAligned) {
    FlatenIndex fi(4, 8, &g_factory);
    auto vec = fi[0];
    ASSERT_NE(vec, nullptr);
    uintptr_t addr = reinterpret_cast<uintptr_t>(vec->Data());
    EXPECT_EQ(addr % 32, 0u) << "Row 0 data is not 32-byte aligned";
}

TEST(FlatenIndexOperator, ReturnsNonNullVector) {
    FlatenIndex fi(3, 4, &g_factory);
    auto vec = fi[0];
    EXPECT_NE(vec, nullptr);
}

TEST(FlatenIndexOperator, ReturnedVectorHasCorrectDimension) {
    FlatenIndex fi(3, 5, &g_factory);
    auto vec = fi[0];
    ASSERT_NE(vec, nullptr);
    EXPECT_EQ(vec->Dimension(), 5u);
}

TEST(FlatenIndexOperator, WriteAndReadBackSingleRow) {
    FlatenIndex fi(1, 4, &g_factory);
    {
        auto vec = fi[0];
        ASSERT_NE(vec, nullptr);
        vec->operator()(0) = 1.1;
        vec->operator()(1) = 2.2;
        vec->operator()(2) = 3.3;
        vec->operator()(3) = 4.4;
    }
    auto vec2 = fi[0];
    ASSERT_NE(vec2, nullptr);
    EXPECT_DOUBLE_EQ(vec2->operator()(0), 1.1);
    EXPECT_DOUBLE_EQ(vec2->operator()(1), 2.2);
    EXPECT_DOUBLE_EQ(vec2->operator()(2), 3.3);
    EXPECT_DOUBLE_EQ(vec2->operator()(3), 4.4);
}

TEST(FlatenIndexOperator, RowsDoNotOverlapEachOther) {
    const std::size_t N = 5, D = 4;
    FlatenIndex fi(N, D, &g_factory);

    // Fill each row with its row index
    for (std::size_t i = 0; i < N; ++i) {
        auto vec = fi[i];
        ASSERT_NE(vec, nullptr);
        for (std::size_t j = 0; j < D; ++j)
            vec->operator()(j) = static_cast<double>(i);
    }

    // Verify no row was clobbered by a neighbour
    for (std::size_t i = 0; i < N; ++i) {
        auto vec = fi[i];
        ASSERT_NE(vec, nullptr);
        for (std::size_t j = 0; j < D; ++j)
            EXPECT_DOUBLE_EQ(vec->operator()(j), static_cast<double>(i))
                << "Corruption at row=" << i << " col=" << j;
    }
}

TEST(FlatenIndexOperator, RowPointersStrideByPadding) {
    FlatenIndex fi(3, 5, &g_factory);   // padding = 8
    auto v0 = fi[0];
    auto v1 = fi[1];
    ASSERT_NE(v0, nullptr);
    ASSERT_NE(v1, nullptr);
    std::ptrdiff_t stride = v1->Data() - v0->Data();
    EXPECT_EQ(static_cast<std::size_t>(stride), fi.padding());
}

TEST(FlatenIndexOperator, NullFactoryReturnsNullptr) {
    FlatenIndex fi(2, 4, nullptr);
    EXPECT_EQ(fi[0], nullptr);
}


TEST(FlatenIndexBounds, ValidLastIndexDoesNotThrow) {
    FlatenIndex fi(3, 4, &g_factory);
    EXPECT_NO_THROW(fi[2]);
}

TEST(FlatenIndexBounds, OutOfRangeThrows) {
    FlatenIndex fi(3, 4, &g_factory);
    EXPECT_THROW(fi[3], std::out_of_range);
}

TEST(FlatenIndexBounds, FarOutOfRangeThrows) {
    FlatenIndex fi(3, 4, &g_factory);
    EXPECT_THROW(fi[100], std::out_of_range);
}


TEST(FlatenIndexLarge, ThousandRowsAccessible) {
    const std::size_t N = 1000, D = 128;
    FlatenIndex fi(N, D, &g_factory);

    EXPECT_EQ(fi.elements(), N);
    EXPECT_EQ(fi.dimension(), D);
    EXPECT_EQ(fi.padding(),   D);   // 128 is a multiple of 4

    fi[0]->operator()(0)       = -1.0;
    fi[N-1]->operator()(D-1)   =  42.0;
    EXPECT_DOUBLE_EQ(fi[0]->operator()(0),      -1.0);
    EXPECT_DOUBLE_EQ(fi[N-1]->operator()(D-1),  42.0);
}


TEST(FlatenIndexEdge, SingleElementSingleDimension) {
    FlatenIndex fi(1, 1, &g_factory);
    EXPECT_EQ(fi.elements(),  1u);
    EXPECT_EQ(fi.dimension(), 1u);
    EXPECT_EQ(fi.padding(),   4u);
    fi[0]->operator()(0) = 3.14;
    EXPECT_DOUBLE_EQ(fi[0]->operator()(0), 3.14);
}

TEST(FlatenIndexEdge, NonAlignedDimensionWriteRead) {
    // dimension=3 → padding=4
    FlatenIndex fi(2, 3, &g_factory);
    fi[0]->operator()(0) = 10.0;
    fi[0]->operator()(2) = 30.0;
    fi[1]->operator()(0) = 40.0;
    fi[1]->operator()(2) = 60.0;

    EXPECT_DOUBLE_EQ(fi[0]->operator()(0), 10.0);
    EXPECT_DOUBLE_EQ(fi[0]->operator()(2), 30.0);
    EXPECT_DOUBLE_EQ(fi[1]->operator()(0), 40.0);
    EXPECT_DOUBLE_EQ(fi[1]->operator()(2), 60.0);
}