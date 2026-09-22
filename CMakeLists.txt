cmake_minimum_required(VERSION 4.2)
project(PandaDB)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

add_library(panda_lib
        MathCompponents/src/Matix/Matrix.cpp
        MathCompponents/src/Matix/Matrix_CPU.cpp
        Utils/config/CASH_SIZE/CASH_SIZE.cpp
        MathCompponents/src/Vector/IVector.cpp
        MathCompponents/includes/Vector/IVector.h
        MathCompponents/src/Vector/Vector_CPU.cpp
        MathCompponents/includes/Vector/Vector_CPU.h
        MathCompponents/includes/Vector/FactoryVector.h
        MathCompponents/src/Vector/FactoryVectorCPU.cpp
        MathCompponents/includes/Vector/FactoryVectorCPU.h
        Metrics/src/IMetrics.cpp
        Metrics/includes/IMetrics.h
        Metrics/src/MetricFactory.cpp
        Metrics/includes/MetricFactory.h
        Metrics/src/EuclideMetrics.cpp
        Metrics/includes/EuclideMetrics.h
        Metrics/src/register.cpp
        Metrics/includes/register.h
        Metrics/src/CosineSimilarityMetric.cpp
        Metrics/includes/CosineSimilarityMetric.h
        Metrics/src/DotMetric.cpp
        Metrics/includes/DotMetric.h
        Data/FlatIndex/src/FlatenIndex.cpp
        Data/FlatIndex/includes/FlatenIndex.h
        Algoritams/src/Sercher.cpp
        Algoritams/includes/Sercher.h
        Algoritams/includes/neighbor.h
        Algoritams/src/neighbor.cpp
        File/includes/FileMenager.h
        File/src/FileMenager.cpp
        File/includes/fileHendler.h
        File/src/fileHendler.cpp
        Utils/config/Block/Block.h
        Utils/config/Block/BLock.cpp
        File/test/fileTest.cpp
        KMeans/CodeBooks/includes/codeBook.h
        Data/VectorStream/includes/IVectorStream.h
        KMeans/CodeBooks/src/codeBook.cpp
        KMeans/CodeBooks/includes/productquntizier.h
        KMeans/CodeBooks/includes/trainer.h
        KMeans/CodeBooks/src/productquntizier.cpp
        KMeans/CodeBooks/src/trainer.cpp
        Data/LinearStorage/includes/linearStorage.h
        Data/LinearStorage/src/linearStorage.cpp
        Data/VectorStream/includes/IVectorReaderStream.h
        Data/VectorStream/src/IVectorReaderStream.cpp
)

target_include_directories(panda_lib PUBLIC MathCompponents/includes)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx2" HAS_AVX2)
check_cxx_compiler_flag("-mfma" HAS_FMA)
if(HAS_AVX2 AND HAS_FMA)
    target_compile_options(panda_lib PRIVATE
            -O3
            -mavx2
            -mfma
    )
endif()

find_package(TBB REQUIRED)
target_link_libraries(panda_lib PUBLIC TBB::tbb)

add_executable(PandaDB main.cpp)
target_link_libraries(PandaDB PRIVATE panda_lib)

add_executable(PandaDB_tests
        MathCompponents/test/matrix_test.cpp
        MathCompponents/test/vector_test.cpp
        Metrics/test/metrics_test.cpp
        Data/FlatIndex/test/FlatIndex_test.cpp
        Algoritams/tests/searcher_test.cpp
        File/test/fileTest.cpp
)
target_link_libraries(PandaDB_tests
        PRIVATE
        panda_lib
        GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(PandaDB_tests)