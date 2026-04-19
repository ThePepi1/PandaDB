cmake_minimum_required(VERSION 4.2)
project(PandaDB)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)
add_library(panda_lib
        Data/src/Matrix.cpp
        Data/src/Matrix_CPU.cpp
)
target_include_directories(panda_lib PUBLIC Data/includes)


add_executable(PandaDB main.cpp)
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