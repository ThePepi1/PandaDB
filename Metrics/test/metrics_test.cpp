#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <cmath>
#include <memory>
#include <string>

#include "Vector/FactoryVector.h"
#include "Vector/FactoryVectorCPU.h"
#include "../includes/MetricFactory.h"
#include "../includes/IMetrics.h"
#include "../includes/register.h"
using namespace std;
using namespace data;
using namespace metrics;


static FactoryVector *factory_vector = new FactoryVectorCPU();
struct MetricParams {
    std::string metric_name;
    size_t      dimension;
    double      a;           // uniform fill of first vector
    double      b;           // uniform fill of second vector
    double      expected;    // ground-truth distance
    double      tol;         // per-case tolerance
};

class MetricAccuracyTest : public ::testing::TestWithParam<MetricParams> {
protected:
    std::unique_ptr<IMetrics> metric;
    std::unique_ptr<IVector>  v1, v2;

    void SetUp() override {
        auto p = GetParam();
        register_builtin_metrics();
        metric = MetricFactory::instance().create_metric(p.metric_name);
        ASSERT_NE(metric, nullptr) << "Unknown metric: " << p.metric_name;
        v1 = factory_vector->createVector(p.dimension, p.a);
        v2 = factory_vector->createVector(p.dimension, p.b);
    }
};

TEST_P(MetricAccuracyTest, Distance) {
    auto p = GetParam();

    auto start    = chrono::steady_clock::now();
    double result = (*metric)(v1.get(), v2.get());
    auto end      = chrono::steady_clock::now();

    EXPECT_NEAR(result, p.expected, p.tol)
        << "[" << p.metric_name << "] dim=" << p.dimension
        << " a=" << p.a << " b=" << p.b
        << " | expected=" << p.expected << " got=" << result;

    cout << "[" << p.metric_name << "] dim=" << p.dimension
         << " | " << chrono::duration_cast<chrono::microseconds>(end - start).count()
         << " us\n";
}
TEST_P(MetricAccuracyTest, SameVectors) {
    int expected = 0;
    if (GetParam().metric_name == "cosine_similarity") {
        expected = 1;
    }
    auto p = GetParam();
    auto start    = chrono::steady_clock::now();
    double result = (*metric)(v1.get(), v1.get());
    auto end      = chrono::steady_clock::now();
    EXPECT_NEAR(result, expected, p.tol) << "Distance between identical vectors should be zero";
}
TEST_P(MetricAccuracyTest, DimensionMismatchThrows) {
    auto p = GetParam();
    auto other = factory_vector->createVector(p.dimension + 1, p.a);
    EXPECT_THROW((*metric)(v1.get(), other.get()), std::invalid_argument);
}
INSTANTIATE_TEST_SUITE_P(
    AccurecyTests,
    MetricAccuracyTest,
    ::testing::Values(
        //  metric         dim      a      b       expected = |a-b|*sqrt(dim)        tol
        MetricParams{"euclide",      1,   2.0,   5.0,   3.0,                      1e-9},
        MetricParams{"euclide",      4,   0.0,   1.0,   2.0,                      1e-9},  // sqrt(4)=2
        MetricParams{"euclide",      9,   0.0,   1.0,   3.0,                      1e-9},  // sqrt(9)=3
        MetricParams{"euclide",     16,   1.5,   2.5,   4.0,                      1e-9},  // sqrt(16)=4
        MetricParams{"euclide",    100,   3.0,   7.0,  40.0,                      1e-6},
        MetricParams{"euclide",   1000,   0.0,   2.0,  63.245553203367585,        1e-6},
        MetricParams{"euclide",   1000,   1.0,   4.0,  94.868329805051381,        1e-6},
        MetricParams{"euclide", 1<<20,    1.0,   2.0,  1024.0,                    1e-4},  // sqrt(2^20)=1024
        MetricParams{"euclide", 1<<27,    4.0,   5.0,  11585.2375,                   1e-3},  // sqrt(2^27)=32768
        MetricParams{"euclide",      7,   3.0,   7.0,  10.583005244258363,        1e-6},  // odd dim
        MetricParams{"euclide",      3,   0.0,   0.0,   0.0,                      1e-9},
        MetricParams{"cosine_similarity",      1,   2.0,   5.0,   1.0,                      1e-9},
        MetricParams{"cosine_similarity",     16,   1.5,   2.5,   1.0,                      1e-9},
        MetricParams{"cosine_similarity",    100,   3.0,   7.0,   1.0,                      1e-6},
        MetricParams{"cosine_similarity",   1000,   1.0,   4.0,   1.0,                      1e-6},
        MetricParams{"cosine_similarity", 1<<20,   1.0,   2.0,   1.0,                      1e-4},
        MetricParams{"cosine_similarity", 1<<27,   4.0,   5.0,   1.0,                      1e-3},
        MetricParams{"cosine_similarity",      7,   3.0,   7.0,   1.0,                      1e-6}
    ),
    [](const testing::TestParamInfo<MetricParams>& info) {
        auto& p = info.param;
        return p.metric_name + "_dim" + to_string(p.dimension)
             + "_a" + to_string(static_cast<int>(p.a * 10))
             + "_b" + to_string(static_cast<int>(p.b * 10));
    }
);
