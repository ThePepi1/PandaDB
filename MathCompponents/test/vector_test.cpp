#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <cmath>

#include "../includes/Vector/Vector_CPU.h"
#include "Vector/FactoryVector.h"
#include "Vector/FactoryVectorCPU.h"
#include "Vector/FactoryVector.h"

using namespace std;
using namespace data;

static void expect_vector_near(IVector &A, IVector &B, double tol = 1e-9) {
    EXPECT_EQ(A.Dimension(), B.Dimension());
    for (size_t i = 0; i < A.Dimension(); i++)
        EXPECT_NEAR(A(i), B(i), tol) << " mismatch at index " << i;
}
static FactoryVector *factory_vector = new FactoryVectorCPU();

class AccessTestVector : public ::testing::Test {
protected:
    unique_ptr<IVector> v;

    void SetUp() override {
        v = factory_vector->createVector(8);
    }
};

TEST_F(AccessTestVector, Constructor) {
    EXPECT_EQ(v->Dimension(), 8);
}

TEST_F(AccessTestVector, ReadWrite) {
    (*v)(3) = 4.2;
    EXPECT_DOUBLE_EQ((*v)(3), 4.2);
}

TEST_F(AccessTestVector, OutOfBoundsThrows) {
    EXPECT_THROW((*v)(100), std::out_of_range);
}


struct VectorParams {
    size_t dimension;
    double a;
    double b;
    double scalar;
};

class ArithmeticOperationTestVector : public ::testing::TestWithParam<VectorParams> {
protected:
    std::unique_ptr<IVector> v1, v2;

    void SetUp() override {
        auto p = GetParam();
        v1 = factory_vector->createVector(p.dimension, p.a);
        v2 = factory_vector->createVector(p.dimension, p.b);
    }
};

TEST_P(ArithmeticOperationTestVector, VectorAddition) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();
    auto result = (*v1) + (*v2);
    auto end = chrono::steady_clock::now();

    std::unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a + p.b);
    expect_vector_near(*result, *expected);

    cout << "VecAdd [dim=" << p.dimension << "]: "
            << chrono::duration_cast<chrono::microseconds>(end - start).count()
            << " us\n";
}

TEST_P(ArithmeticOperationTestVector, VectorSubtraction) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();
    auto result = (*v1) - (*v2);
    auto end = chrono::steady_clock::now();

    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a - p.b);
    expect_vector_near(*result, *expected);

    cout << "VecSub [dim=" << p.dimension << "]: "
            << chrono::duration_cast<chrono::microseconds>(end - start).count()
            << " us\n";
}

TEST_P(ArithmeticOperationTestVector, ScalarAddition) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();
    auto result = (*v1) + p.scalar;
    auto end = chrono::steady_clock::now();

    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a + p.scalar);
    expect_vector_near(*result, *expected);

    cout << "ScalarAdd [dim=" << p.dimension << "]: "
            << chrono::duration_cast<chrono::microseconds>(end - start).count()
            << " us\n";
}

TEST_P(ArithmeticOperationTestVector, ScalarMultiply) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();
    auto result = (*v1) * p.scalar;
    auto end = chrono::steady_clock::now();

    unique_ptr<IVector> expected= factory_vector->createVector(p.dimension, p.a * p.scalar);
    expect_vector_near(*result, *expected);

    cout << "ScalarMul [dim=" << p.dimension << "]: "
            << chrono::duration_cast<chrono::microseconds>(end - start).count()
            << " us\n";
}
TEST_P(ArithmeticOperationTestVector, ScalarDivision) {
    auto p = GetParam();
    if (std::abs(p.scalar) < 1e-10) {
        GTEST_SKIP() << "Skipping division by zero case";
    }
    auto start = chrono::steady_clock::now();
    auto result = (*v1) / p.scalar;
    auto end = chrono::steady_clock::now();
    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a / p.scalar);
    expect_vector_near(*result, *expected);
    cout << "ScalarDiv [dim=" << p.dimension << "]: "
    << chrono::duration_cast<chrono::microseconds>(end - start).count() << " us\n";
}

TEST_P(ArithmeticOperationTestVector, DotProduct) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();
    double result = (*v1) * (*v2);
    auto end = chrono::steady_clock::now();

    // dot(a_vec, b_vec) = a * b * dimension (all elements are a and b)
    double expected = p.a * p.b * static_cast<double>(p.dimension);
    EXPECT_NEAR(result, expected, 1e-9);

    cout << "Dot [dim=" << p.dimension << "]: "
            << chrono::duration_cast<chrono::microseconds>(end - start).count()
            << " us\n";
}

TEST_P(ArithmeticOperationTestVector, DimensionMismatchThrows) {
    unique_ptr<IVector> other= factory_vector->createVector(GetParam().dimension + 1, 1.0);
    EXPECT_THROW((*v1) + *other, std::invalid_argument);
    EXPECT_THROW((*v1) - *other, std::invalid_argument);
    EXPECT_THROW((*v1) * *other, std::invalid_argument);
}

INSTANTIATE_TEST_SUITE_P(
    ArithmeticTests,
    ArithmeticOperationTestVector,
    ::testing::Values(
        VectorParams{1 << 27 , 4.0, 5.0, 1.0}, // very large —  SIMD path
        VectorParams{1 << 20, 1.0, 2.0, 3.0}, // large — SIMD path
        VectorParams{1000, 2.0, 3.0, 0.5},
        VectorParams{16, 1.5, 2.5, 2.0}, // small — boundary
        VectorParams{7, 3.0, 4.0, 1.0}, // odd size — scalar tail
        VectorParams{1, 5.0, 6.0, 2.0} // minimum

    )
);

class CrossProductTestVector : public ::testing::Test {
protected:
    std::unique_ptr<IVector> v1, v2;
    void SetUp() override {
        v1 = factory_vector->createVector(3);
        v2 = factory_vector->createVector(3);
    }
};

TEST_F(CrossProductTestVector, BasisVectors_X_cross_Y_equals_Z) {
    (*v1)(0) = 1; (*v1)(1) = 0; (*v1)(2) = 0; // X
    (*v2)(0) = 0; (*v2)(1) = 1; (*v2)(2) = 0; // Y
    auto result = (*v1) ^ (*v2);
    EXPECT_NEAR((*result)(0), 0.0, 1e-9);
    EXPECT_NEAR((*result)(1), 0.0, 1e-9);
    EXPECT_NEAR((*result)(2), 1.0, 1e-9);
}

TEST_F(CrossProductTestVector, AntiCommutativity) {
    (*v1)(0) = 1; (*v1)(1) = 2; (*v1)(2) = 3;
    (*v2)(0) = 4; (*v2)(1) = 5; (*v2)(2) = 6;
    auto ab = (*v1) ^ (*v2);
    auto ba = (*v2) ^ (*v1);
    for (size_t i = 0; i < 3; i++)
        EXPECT_NEAR((*ab)(i), -(*ba)(i), 1e-9) << " at index " << i;
}

TEST_F(CrossProductTestVector, ParallelVectorsGiveZero) {
    (*v1)(0) = 2; (*v1)(1) = 4; (*v1)(2) = 6;
    (*v2)(0) = 1; (*v2)(1) = 2; (*v2)(2) = 3; // parallel to v1
    auto result = (*v1) ^ (*v2);
    for (size_t i = 0; i < 3; i++)
        EXPECT_NEAR((*result)(i), 0.0, 1e-9) << " at index " << i;
}

TEST_F(CrossProductTestVector, ResultIsPerpendicularToBothInputs) {
    (*v1)(0) = 1; (*v1)(1) = 2; (*v1)(2) = 3;
    (*v2)(0) = 4; (*v2)(1) = 5; (*v2)(2) = 6;
    auto result = (*v1) ^ (*v2);
    EXPECT_NEAR((*result) * (*v1), 0.0, 1e-9);
    EXPECT_NEAR((*result) * (*v2), 0.0, 1e-9);
}

TEST_F(CrossProductTestVector, NonThreeDimensionalThrows) {
    auto a = factory_vector->createVector(2, 1.0);
    auto b = factory_vector->createVector(2, 1.0);
    EXPECT_THROW((*a) ^ (*b), std::invalid_argument);

    auto c = factory_vector->createVector(4, 1.0);
    auto d = factory_vector->createVector(4, 1.0);
    EXPECT_THROW((*c) ^ (*d), std::invalid_argument);
}

TEST_P(ArithmeticOperationTestVector, Norm) {
    auto p = GetParam();
    auto start = chrono::steady_clock::now();
    double result = v1->norm();
    auto end = chrono::steady_clock::now();
    double expected = std::abs(p.a) * std::sqrt(static_cast<double>(p.dimension));
    EXPECT_NEAR(result, expected, 1e-6);
    cout << "Norm [dim=" << p.dimension << "]: "
         << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";
}

TEST_P(ArithmeticOperationTestVector, NormIsNonNegative) {
    EXPECT_GE(v1->norm(), 0.0);
    EXPECT_GE(v2->norm(), 0.0);
}

TEST_P(ArithmeticOperationTestVector, NormalizeResultHasUnitNorm) {
    auto p = GetParam();
    auto start = chrono::steady_clock::now();
    auto result = v1->normalize();
    auto end = chrono::steady_clock::now();
    cout << "Normalize [dim=" << p.dimension << "]: "
         << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";
    EXPECT_NEAR(result->norm(), 1.0, 1e-9);
}

TEST_P(ArithmeticOperationTestVector, NormalizeDirectionPreserved) {
    double n = v1->norm();
    auto result = v1->normalize();
    auto scaled = (*result) * n;
    for (size_t i = 0; i < v1->Dimension(); i++)
        EXPECT_NEAR((*scaled)(i), (*v1)(i), 1e-6) << " at index " << i;
}

TEST(NormalizeEdgeTest, ZeroVectorThrows) {
    auto v = factory_vector->createVector(4, 0.0);
    EXPECT_THROW(v->normalize(), std::runtime_error);
}

TEST(ScalarDivisionEdgeTest, DivisionByZeroThrows) {
    auto v = factory_vector->createVector(10, 1.0);
    EXPECT_THROW((*v) / 0.0, std::runtime_error);
}
class EqualityTestVector : public ::testing::TestWithParam<VectorParams> {
protected:
    std::unique_ptr<IVector> v1, v2;
    void SetUp() override {
        auto p = GetParam();
        v1 = factory_vector->createVector(p.dimension, p.a);
        v2 = factory_vector->createVector(p.dimension, p.a); // same fill as v1
    }
};

TEST_P(EqualityTestVector, IdenticalVectorsAreEqual) {
    EXPECT_TRUE((*v1) == (*v2));
}

TEST_P(EqualityTestVector, VectorEqualsItself) {
    EXPECT_TRUE((*v1) == (*v1));
}

TEST_P(EqualityTestVector, ModifiedVectorIsNotEqual) {
    (*v2)(0) += 1.0;
    EXPECT_FALSE((*v1) == (*v2));
}

INSTANTIATE_TEST_SUITE_P(
    EqualityTests,
    EqualityTestVector,
    ::testing::Values(
        VectorParams{1, 5.0, 0.0, 0.0},
        VectorParams{7, 3.0, 0.0, 0.0},
        VectorParams{16, 1.5, 0.0, 0.0},
        VectorParams{1000, 2.0, 0.0, 0.0}
    )
);

// NOTE: assumes mismatched dimensions throw, matching the other operators.
// If operator== is meant to just return false instead, swap this for
// EXPECT_FALSE((*a) == (*b)).
TEST(EqualityEdgeTest, DimensionMismatchThrows) {
    auto a = factory_vector->createVector(4, 1.0);
    auto b = factory_vector->createVector(5, 1.0);
    EXPECT_EQ((*a) == (*b), false);
}

class AssignmentTestVector : public ::testing::TestWithParam<VectorParams> {
protected:
    std::unique_ptr<IVector> v1, v2;
    void SetUp() override {
        auto p = GetParam();
        v1 = factory_vector->createVector(p.dimension, p.a);
        v2 = factory_vector->createVector(p.dimension, p.b);
    }
};

TEST_P(AssignmentTestVector, OperatorSyntaxAssignsValues) {
    *v1 = *v2;
    expect_vector_near(*v1, *v2);
}

TEST_P(AssignmentTestVector, AssignmentDoesNotAliasSource) {
    *v1 = *v2;
    (*v2)(0) += 100.0;
    EXPECT_NE((*v1)(0), (*v2)(0));
}

TEST_P(AssignmentTestVector, SelfAssignmentIsSafe) {
    *v1 = *v1;
    unique_ptr<IVector> expected = factory_vector->createVector(GetParam().dimension, GetParam().a);
    expect_vector_near(*v1, *expected);
}

INSTANTIATE_TEST_SUITE_P(
    AssignmentTests,
    AssignmentTestVector,
    ::testing::Values(
        VectorParams{1, 5.0, 6.0, 0.0},
        VectorParams{7, 3.0, 4.0, 0.0},
        VectorParams{16, 1.5, 2.5, 0.0},
        VectorParams{1000, 2.0, 3.0, 0.0}
    )
);

// NOTE: same assumption as above — adjust if operator= is meant to
// silently no-op or resize instead of throwing on a dimension mismatch.
TEST(AssignmentEdgeTest, DimensionMismatchThrows) {
    auto a = factory_vector->createVector(4, 1.0);
    auto b = factory_vector->createVector(5, 1.0);
    EXPECT_THROW(*a = *b, std::invalid_argument);
}
TEST_P(ArithmeticOperationTestVector, CompoundVectorAddition) {
    auto p = GetParam();
    IVector& ref = (*v1) += (*v2);

    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a + p.b);
    expect_vector_near(*v1, *expected);
    EXPECT_EQ(&ref, v1.get()); // should return *this, not a copy
}

TEST_P(ArithmeticOperationTestVector, CompoundVectorSubtraction) {
    auto p = GetParam();
    IVector& ref = (*v1) -= (*v2);

    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a - p.b);
    expect_vector_near(*v1, *expected);
    EXPECT_EQ(&ref, v1.get());
}

TEST_P(ArithmeticOperationTestVector, CompoundScalarAddition) {
    auto p = GetParam();
    IVector& ref = (*v1) += p.scalar;

    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a + p.scalar);
    expect_vector_near(*v1, *expected);
    EXPECT_EQ(&ref, v1.get());
}

TEST_P(ArithmeticOperationTestVector, CompoundScalarSubtraction) {
    auto p = GetParam();
    IVector& ref = (*v1) -= p.scalar;

    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a - p.scalar);
    expect_vector_near(*v1, *expected);
    EXPECT_EQ(&ref, v1.get());
}

TEST_P(ArithmeticOperationTestVector, CompoundScalarMultiply) {
    auto p = GetParam();
    IVector& ref = (*v1) *= p.scalar;

    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a * p.scalar);
    expect_vector_near(*v1, *expected);
    EXPECT_EQ(&ref, v1.get());
}

TEST_P(ArithmeticOperationTestVector, CompoundScalarDivision) {
    auto p = GetParam();
    if (std::abs(p.scalar) < 1e-10) {
        GTEST_SKIP() << "Skipping division by zero case";
    }
    IVector& ref = (*v1) /= p.scalar;

    unique_ptr<IVector> expected = factory_vector->createVector(p.dimension, p.a / p.scalar);
    expect_vector_near(*v1, *expected);
    EXPECT_EQ(&ref, v1.get());
}

TEST_P(ArithmeticOperationTestVector, CompoundDimensionMismatchThrows) {
    unique_ptr<IVector> other = factory_vector->createVector(GetParam().dimension + 1, 1.0);
    EXPECT_THROW((*v1) += *other, std::invalid_argument);
    EXPECT_THROW((*v1) -= *other, std::invalid_argument);
}