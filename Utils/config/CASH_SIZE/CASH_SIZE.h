// includes/config/Config.h
#pragma once
#include <cstddef>

namespace config {
    extern const size_t OPTIMAL_BLOCK;

    extern const size_t L1_SIZE;
    extern const size_t L2_SIZE;
    extern const size_t L3_SIZE;

    extern const size_t ROWS_PER_L2_BLOCK = 180;
    extern const size_t COLS_PER_L1_BLOCK = 96;
    extern const size_t SHARED_PER_CACHE_BLOCK = 240;

    extern const size_t ROWS_PER_REGISTER = 4;
    extern const size_t COLS_PER_REGISTER = 2;
}