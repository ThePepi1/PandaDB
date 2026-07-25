#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>

#include "Vector/FactoryVectorCPU.h"
#include "../includes/Vector/FactoryVector.h"
#include "../../Metrics/includes/IMetrics.h"
#include "../../Metrics/includes/EuclideMetrics.h"
#include "../../Data/FlatIndex/includes/FlatenIndex.h"
#include "../includes/Sercher.h"
#include  "../../Metrics/includes/register.h"
#include  "../../Metrics/includes/MetricFactory.h"


using namespace std;
using namespace data;
using namespace metrics;


struct SearchParams {
    string   label;
    size_t   num_elements;
    size_t   dimensions;
    size_t   k;
    uint32_t seed;
};


class SearchTest : public ::testing::TestWithParam<SearchParams> {
protected:
    FactoryVectorCPU               factory;
    unique_ptr<IMetrics>           metric;
    unique_ptr<DataStructures::FlatenIndex> index;
    unique_ptr<IVector>            query;
    vector<double>                 ground_truth_distances; // sorted, size=k

    void SetUp() override {
        auto p = GetParam();
        register_builtin_metrics();
       metric =  MetricFactory::instance().create_metric("euclide");

        mt19937 rng(p.seed);
        uniform_real_distribution<double> dist(0.0, 100.0);

        index = make_unique<DataStructures::FlatenIndex>(
            p.num_elements, p.dimensions, &factory
        );

        for (size_t i = 0; i < p.num_elements; i++) {
            double* row = index->row(i);
            for (size_t d = 0; d < p.dimensions; d++)
                row[d] = dist(rng);
            for (size_t d = p.dimensions; d < index->padding(); d++)
                row[d] = 0.0;
        }

        query = factory.createVector(p.dimensions);
        for (size_t d = 0; d < p.dimensions; d++)
            (*query)(d) = dist(rng);

        // Brute-force ground truth
        vector<double> all_distances(p.num_elements);
        for (size_t i = 0; i < p.num_elements; i++) {
            auto elem = (*index)[i];
            all_distances[i] = (*metric)(*elem, *query);
        }
        sort(all_distances.begin(), all_distances.end());
        ground_truth_distances.assign(all_distances.begin(),
                                      all_distances.begin() + p.k);
    }

    vector<double> sorted_distances(data::neighbor* result, size_t k) {
        vector<double> out(k);
        for (size_t i = 0; i < k; i++)
            out[i] = result[i].getValue();
        sort(out.begin(), out.end());
        return out;
    }
};


TEST_P(SearchTest, SerialReturnsKResults) {
    auto p = GetParam();
    data::neighbor* result = algorithm::Search(*index, *query, p.k, *metric);
    ASSERT_NE(result, nullptr);
    delete[] result;
}

TEST_P(SearchTest, SerialCorrectness) {
    auto p = GetParam();
    data::neighbor* result = algorithm::Search(*index, *query, p.k, *metric);

    auto got = sorted_distances(result, p.k);
    for (size_t i = 0; i < p.k; i++) {
        EXPECT_NEAR(got[i], ground_truth_distances[i], 1e-6)
            << "Serial mismatch at rank " << i
            << " expected=" << ground_truth_distances[i]
            << " got=" << got[i];
    }
    delete[] result;
}

TEST_P(SearchTest, SerialPerformance) {
    auto p = GetParam();
    auto t0 = chrono::steady_clock::now();
    data::neighbor* result = algorithm::Search(*index, *query, p.k, *metric);
    auto ms = chrono::duration_cast<chrono::milliseconds>(
                  chrono::steady_clock::now() - t0).count();

    cout << "[Serial][" << p.label << "] "
         << p.num_elements << " elems x " << p.dimensions << " dims"
         << " k=" << p.k << " | " << ms << " ms\n";

    delete[] result;
}

// ─── Parallel tests ──────────────────────────────────────────────────────────

TEST_P(SearchTest, ParallelReturnsKResults) {
    auto p = GetParam();
    data::neighbor* result = algorithm::SearchParrallel(*index, *query, p.k, *metric);
    ASSERT_NE(result, nullptr);
    delete[] result;
}

TEST_P(SearchTest, ParallelCorrectness) {
    auto p = GetParam();
    data::neighbor* result = algorithm::SearchParrallel(*index, *query, p.k, *metric);

    auto got = sorted_distances(result, p.k);
    for (size_t i = 0; i < p.k; i++) {
        EXPECT_NEAR(got[i], ground_truth_distances[i], 1e-6)
            << "Parallel mismatch at rank " << i
            << " expected=" << ground_truth_distances[i]
            << " got=" << got[i];
    }
    delete[] result;
}

TEST_P(SearchTest, ParallelPerformance) {
    auto p = GetParam();
    auto t0 = chrono::steady_clock::now();
    data::neighbor* result = algorithm::SearchParrallel(*index, *query, p.k, *metric);
    auto ms = chrono::duration_cast<chrono::milliseconds>(
                  chrono::steady_clock::now() - t0).count();

    cout << "[Parallel][" << p.label << "] "
         << p.num_elements << " elems x " << p.dimensions << " dims"
         << " k=" << p.k << " | " << ms << " ms\n";

    delete[] result;
}

// ─── Agreement + speedup ─────────────────────────────────────────────────────

TEST_P(SearchTest, SerialAndParallelAgree) {
    auto p = GetParam();
    data::neighbor* serial   = algorithm::Search(*index, *query, p.k, *metric);
    data::neighbor* parallel = algorithm::SearchParrallel(*index, *query, p.k, *metric);

    auto sd = sorted_distances(serial,   p.k);
    auto pd = sorted_distances(parallel, p.k);

    for (size_t i = 0; i < p.k; i++) {
        EXPECT_NEAR(sd[i], pd[i], 1e-6)
            << "Diverge at rank " << i
            << " serial=" << sd[i] << " parallel=" << pd[i];
    }
    delete[] serial;
    delete[] parallel;
}

TEST_P(SearchTest, ParallelFasterThanSerial) {
    auto p = GetParam();

    auto s0 = chrono::steady_clock::now();
    data::neighbor* sr = algorithm::Search(*index, *query, p.k, *metric);
    auto serial_ms = chrono::duration_cast<chrono::milliseconds>(
                         chrono::steady_clock::now() - s0).count();

    auto p0 = chrono::steady_clock::now();
    data::neighbor* pr = algorithm::SearchParrallel(*index, *query, p.k, *metric);
    auto parallel_ms = chrono::duration_cast<chrono::milliseconds>(
                           chrono::steady_clock::now() - p0).count();

    double speedup = serial_ms > 0 ? (double)serial_ms / parallel_ms : 0.0;
    cout << "[Speedup][" << p.label << "]"
         << " serial=" << serial_ms << "ms"
         << " parallel=" << parallel_ms << "ms"
         << " speedup=" << speedup << "x\n";

    EXPECT_LE(parallel_ms, serial_ms)
        << "Parallel should not be slower than serial";

    delete[] sr;
    delete[] pr;
}

// ─── Instantiation ───────────────────────────────────────────────────────────

INSTANTIATE_TEST_SUITE_P(
    SearchSuite,
    SearchTest,
    ::testing::Values(
        SearchParams{"smoke",   10000, 1000, 10, 42},
        SearchParams{"small",     100,   16,  5, 42},
        SearchParams{"large",  100000,  128, 20, 99},
        SearchParams{"huge", 1000000,  256, 50, 123}
    ),
    [](const testing::TestParamInfo<SearchParams>& info) {
        return info.param.label;
    }
);