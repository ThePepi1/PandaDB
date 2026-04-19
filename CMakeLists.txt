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
        Data/src/Matrix.cpp
        Data/src/Matrix_CPU.cpp
        Utils/config/CASH_SIZE/CASH_SIZE.cpp
)

# Include headers
target_include_directories(panda_lib PUBLIC Data/includes)


include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-mavx2" HAS_AVX2)
check_cxx_compiler_flag("-mfma" HAS_FMA)

if(HAS_AVX2 AND HAS_FMA)
    target_compile_options(panda_lib PRIVATE -mavx2 -mfma)
endif()

find_package(TBB REQUIRED)

target_link_libraries(panda_lib PUBLIC TBB::tbb)


add_executable(PandaDB main.cpp)

target_link_libraries(PandaDB PRIVATE panda_lib)


add_executable(PandaDB_tests
        Data/test/matrix_test.cpp
)

target_link_libraries(PandaDB_tests
        PRIVATE
        panda_lib
        GTest::gtest_main
)


include(GoogleTest)
gtest_discover_tests(PandaDB_tests)