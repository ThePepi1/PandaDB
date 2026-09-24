#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <memory>

#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "../../Data/VectorStream/includes/IVectorStream.h"
#include "../includes/trainer.h"
#include "../includes/Vector/FactoryVectorCPU.h"
#include "../includes/productquntizier.h"
#include "../includes/codeBook.h"
#include "../../Metrics/includes/MetricFactory.h"
#include "../../Metrics/includes/register.h"

namespace {
   
    const std::string kBenchmarkFilePath = "/home/pepi/Documents/projects/PandaDB/downloadedDataTest/testVector.storage";
}

class ProductQuantizerTrainPerfTest : public ::testing::Test {
protected:
    std::unique_ptr<DataStructures::LinearStorage> storage;

    void SetUp() override {
        
        storage = std::make_unique<DataStructures::LinearStorage>(kBenchmarkFilePath);
        ASSERT_GT(storage->getSize(), 0) << "Fajl je prazan ili EndMarker nije pronadjen (findEnd() vratio 0).";
        metrics::register_builtin_metrics();    
    }
};

TEST_F(ProductQuantizerTrainPerfTest, TrainSpeedOn1MVectors) {
    data::FactoryVectorCPU factory = data::FactoryVectorCPU();
     // ceo fajl, od 0 do size
    
    auto metrics = metrics::MetricFactory::instance().create_metric("euclide");
    constexpr int kMaxIterations = 20;
    auto trianer = KMeans::Trainer();
    auto start = std::chrono::high_resolution_clock::now();
    auto stream = storage->getVectorStream(1.0, factory);// 1.0 means the whole stream
    KMeans::ProductQuantizer pq(8, 256, stream->getDim(), factory);
    KMeans::CodeBook codeBook( 256, stream->getDim(), factory); 
    trianer.train(pq, codeBook, *stream, *metrics, kMaxIterations);
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "train() nad " << storage->getSize() << " vektora (dim=" << storage->getDim()
              << "), maxIterations=" << kMaxIterations
              << " -> " << elapsedMs << " ms ("
              << (elapsedMs / 1000.0) << " s)\n";
    SUCCEED();
}