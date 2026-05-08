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

find_package(OpenCL REQUIRED)

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
)

target_include_directories(panda_lib PUBLIC MathCompponents/includes)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx2" HAS_AVX2)
check_cxx_compiler_flag("-mfma" HAS_FMA)
if(HAS_AVX2 AND HAS_FMA)
    target_compile_options(panda_lib PRIVATE -mavx2 -mfma)
endif()

find_package(TBB REQUIRED)
target_link_libraries(panda_lib PUBLIC TBB::tbb OpenCL::OpenCL)
target_compile_definitions(panda_lib PUBLIC USE_OPENCL)

add_executable(PandaDB main.cpp)
target_link_libraries(PandaDB PRIVATE panda_lib)

add_executable(PandaDB_tests
        MathCompponents/test/matrix_test.cpp
        MathCompponents/test/vector_test.cpp
        Metrics/test/metrics_test.cpp
)
target_link_libraries(PandaDB_tests
        PRIVATE
        panda_lib
        GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(PandaDB_tests)