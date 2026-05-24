// test_FlatenIndex.cpp
// Build: g++ -std=c++17 test_FlatenIndex.cpp FlatenIndex.cpp -lgtest -lgtest_main -pthread -o test_flaten
// Run:   ./test_flaten

#include <gtest/gtest.h>
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include "../includes/FlatenIndex.h"
namespace Data {

    using Data::FlatenIndex;


    TEST(FlatenIndexConstruction, StoresElementCount) {
        FlatenIndex fi(10, 4);
        EXPECT_EQ(fi.elements(), 10u);
    }

    TEST(FlatenIndexConstruction, StoresDimension) {
        FlatenIndex fi(10, 4);
        EXPECT_EQ(fi.dimension(), 4u);
    }

    TEST(FlatenIndexPadding, AlreadyAligned_NoChange) {
        FlatenIndex fi(1, 4);
        EXPECT_EQ(fi.padding(), 4u);   // 4 is already a multiple of 4
    }

    TEST(FlatenIndexPadding, AlreadyAligned_8) {
        FlatenIndex fi(1, 8);
        EXPECT_EQ(fi.padding(), 8u);
    }

    TEST(FlatenIndexPadding, NeedsOnePad) {
        FlatenIndex fi(1, 5);
        EXPECT_EQ(fi.padding(), 8u);   // next multiple of 4 after 5
    }

    TEST(FlatenIndexPadding, NeedsTwoPad) {
        FlatenIndex fi(1, 6);
        EXPECT_EQ(fi.padding(), 8u);
    }

    TEST(FlatenIndexPadding, NeedsThreePad) {
        FlatenIndex fi(1, 7);
        EXPECT_EQ(fi.padding(), 8u);
    }

    TEST(FlatenIndexPadding, DimensionOne) {
        FlatenIndex fi(1, 1);
        EXPECT_EQ(fi.padding(), 4u);
    }

    TEST(FlatenIndexPadding, DimensionThree) {
        FlatenIndex fi(1, 3);
        EXPECT_EQ(fi.padding(), 4u);
    }

    TEST(FlatenIndexPadding, LargeDimension) {
        FlatenIndex fi(1, 100);
        EXPECT_EQ(fi.padding(), 100u); // 100 is already a multiple of 4
    }

    TEST(FlatenIndexPadding, LargeNonAlignedDimension) {
        FlatenIndex fi(1, 101);
        EXPECT_EQ(fi.padding(), 104u);
    }


    TEST(FlatenIndexAlignment, RowZeroIs64ByteAligned) {
        FlatenIndex fi(4, 8);
        uintptr_t addr = reinterpret_cast<uintptr_t>(fi[0]);
        EXPECT_EQ(addr % 64, 0u) << "Row 0 pointer is not 64-byte aligned";
    }

    TEST(FlatenIndexOperator, RowPointersDifferByPaddedStride) {
        FlatenIndex fi(3, 5);          // padding = 8
        std::ptrdiff_t stride = fi[1] - fi[0];
        EXPECT_EQ(static_cast<std::size_t>(stride), fi.padding());
    }

    TEST(FlatenIndexOperator, WriteAndReadBackSingleRow) {
        FlatenIndex fi(1, 4);
        double* row = fi[0];
        row[0] = 1.1; row[1] = 2.2; row[2] = 3.3; row[3] = 4.4;
        EXPECT_DOUBLE_EQ(fi[0][0], 1.1);
        EXPECT_DOUBLE_EQ(fi[0][1], 2.2);
        EXPECT_DOUBLE_EQ(fi[0][2], 3.3);
        EXPECT_DOUBLE_EQ(fi[0][3], 4.4);
    }

    TEST(FlatenIndexOperator, RowsDoNotOverlap) {
        const std::size_t N = 5, D = 4;
        FlatenIndex fi(N, D);

        // Fill each row with its row index
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < D; ++j)
                fi[i][j] = static_cast<double>(i);

        // Verify no row was clobbered by a neighbour
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < D; ++j)
                EXPECT_DOUBLE_EQ(fi[i][j], static_cast<double>(i))
                    << "Corruption at row " << i << " col " << j;
    }

    TEST(FlatenIndexOperator, WriteAndReadNonAlignedDimension) {
        FlatenIndex fi(2, 3);
        fi[0][0] = 10.0; fi[0][1] = 20.0; fi[0][2] = 30.0;
        fi[1][0] = 40.0; fi[1][1] = 50.0; fi[1][2] = 60.0;

        EXPECT_DOUBLE_EQ(fi[0][0], 10.0);
        EXPECT_DOUBLE_EQ(fi[0][2], 30.0);
        EXPECT_DOUBLE_EQ(fi[1][0], 40.0);
        EXPECT_DOUBLE_EQ(fi[1][2], 60.0);
    }

    TEST(FlatenIndexLarge, CanAllocateAndAccessThousandRows) {
        const std::size_t N = 1000, D = 128;
        FlatenIndex fi(N, D);
        EXPECT_EQ(fi.elements(), N);
        EXPECT_EQ(fi.dimension(), D);
        EXPECT_EQ(fi.padding(),   D);   // 128 is a multiple of 4

        fi[0][0]       = -1.0;
        fi[N - 1][D - 1] = 42.0;
        EXPECT_DOUBLE_EQ(fi[0][0],         -1.0);
        EXPECT_DOUBLE_EQ(fi[N-1][D-1],     42.0);
    }

    TEST(FlatenIndexEdge, SingleElementSingleDimension) {
        FlatenIndex fi(1, 1);
        fi[0][0] = 3.14;
        EXPECT_DOUBLE_EQ(fi[0][0], 3.14);
        EXPECT_EQ(fi.elements(),  1u);
        EXPECT_EQ(fi.dimension(), 1u);
        EXPECT_EQ(fi.padding(),   4u);
    }

    TEST(FlatenIndexEdge, RowPointerConsistentAcrossMultipleCalls) {
        FlatenIndex fi(3, 4);
        double* p1 = fi[1];
        double* p2 = fi[1];
        EXPECT_EQ(p1, p2);
    }
}