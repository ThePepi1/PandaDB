#include <gtest/gtest.h>
#include <chrono>
#include <iostream>

#include "../includes/Matrix.h"
#include "../includes/Matrix_CPU.h"

using namespace std;
using namespace data;


static void expect_matrix_near(IMatrix* A, IMatrix* B, double tol = 1e-9) {
    EXPECT_EQ(A->Rows(), B->Rows());
    EXPECT_EQ(A->Columns(), B->Columns());

    for (size_t r = 0; r < A->Rows(); r++)
        for (size_t c = 0; c < A->Columns(); c++)
            EXPECT_NEAR((*A)(r,c), (*B)(r,c), tol)
                << " mismatch at (" << r << "," << c << ")";
}


class AccessTest : public ::testing::Test {
protected:
    IMatrix* m;

    void SetUp() override {
        m = new Matrix_CPU(4, 7);
    }

    void TearDown() override {
        delete m;
    }
};


TEST_F(AccessTest, Constructor) {
    EXPECT_EQ(m->Rows(), 4);
    EXPECT_EQ(m->Columns(), 7);
}

TEST_F(AccessTest, ReadWrite) {
    (*m)(1, 2) = 7.5;
    EXPECT_DOUBLE_EQ((*m)(1, 2), 7.5);
}

TEST_F(AccessTest, AtThrowsOutOfBounds) {
    EXPECT_THROW((*m)(10, 0), std::out_of_range);
    EXPECT_THROW((*m)(0, 10), std::out_of_range);
}

struct ArithmeticParams {
    size_t rows;
    size_t cols;
    double a;
    double b;
    double scalar;
};


class ArithmeticOperationTest : public ::testing::TestWithParam<ArithmeticParams> {
protected:
    IMatrix *m1, *m2;

    void SetUp() override {
        auto p = GetParam();

        m1 = new Matrix_CPU(p.rows, p.cols, p.a);
        m2 = new Matrix_CPU(p.rows, p.cols, p.b);
    }

    void TearDown() override {
        delete m1;
        delete m2;
    }
};


TEST_P(ArithmeticOperationTest, Addition) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();

    IMatrix* result = (*m1) + (*m2);

    auto end = chrono::steady_clock::now();

    IMatrix* expected = new Matrix_CPU(p.rows, p.cols, p.a + p.b);

    expect_matrix_near(result, expected);

    cout << "Add: "
         << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";

    delete result;
    delete expected;
}

TEST_P(ArithmeticOperationTest, Subtraction) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();

    IMatrix* result = (*m1) - (*m2);

    auto end = chrono::steady_clock::now();

    IMatrix* expected = new Matrix_CPU(p.rows, p.cols, p.a - p.b);

    expect_matrix_near(result, expected);

    cout << "Sub: "
         << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";

    delete result;
    delete expected;
}

TEST_P(ArithmeticOperationTest, ScalarAddition) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();

    IMatrix* result = (*m1) + p.scalar;

    auto end = chrono::steady_clock::now();

    IMatrix* expected = new Matrix_CPU(p.rows, p.cols, p.a + p.scalar);

    expect_matrix_near(result, expected);

    cout << "ScalarAdd: "
         << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";

    delete result;
    delete expected;
}

TEST_P(ArithmeticOperationTest, ScalarMultiply) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();

    IMatrix* result = (*m1) * p.scalar;

    auto end = chrono::steady_clock::now();

    IMatrix* expected = new Matrix_CPU(p.rows, p.cols, p.a * p.scalar);

    expect_matrix_near(result, expected);

    cout << "ScalarMul: "
         << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";

    delete result;
    delete expected;
}
TEST_P(ArithmeticOperationTest, Transpose) {
    auto p = GetParam();

    auto start = chrono::steady_clock::now();

    IMatrix* result = m1->transpose();

    auto end = chrono::steady_clock::now();

    EXPECT_EQ(result->Rows(), p.cols);
    EXPECT_EQ(result->Columns(), p.rows);

    // correctness check
    for (size_t r = 0; r < result->Rows(); r++) {
        for (size_t c = 0; c < result->Columns(); c++) {
            EXPECT_NEAR((*result)(r, c), (*m1)(c, r), 1e-9);
        }
    }

    cout << "Transpose: "
         << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";

    delete result;
}
TEST_P(ArithmeticOperationTest, Multiply) {
    auto start = chrono::steady_clock::now();

    IMatrix* result = (*m1) * (*(m2->transpose()));

    auto end = chrono::steady_clock::now();

    size_t rows = m1->Rows();
    size_t cols = m2->Rows();

    double a = (*m1)(0,0);
    double b = (*m2)(0,0);

    double value = a * b * m1->Columns();

    IMatrix* expected = new Matrix_CPU(rows, cols, value);

    expect_matrix_near(result, expected);

    cout << "Mul: "
         << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";

    delete result;
    delete expected;
}


INSTANTIATE_TEST_SUITE_P(
    ArithmeticTests,
    ArithmeticOperationTest,
    ::testing::Values(
        ArithmeticParams{2080, 2080, 1, 2, 1},
        ArithmeticParams{200, 200, 1, 2, 1},
        ArithmeticParams{100, 100 , 3 , 4 , 1},
        ArithmeticParams{4, 7, 1, 2, 1}
    )
);
