// includes/config/Config.h
#pragma once
#include <cstddef>

namespace config {
    extern const size_t OPTIMAL_BLOCK;

    size_t getL1Size();
    size_t getL2Size();
    size_t getL3Size();

    extern const size_t ROWS_PER_L2_BLOCK ; // Mc
    extern const size_t COLS_PER_L1_BLOCK ; // Nc
    extern const size_t SHARED_PER_CACHE_BLOCK; // Kc

    inline const size_t ROWS_PER_REGISTER = 4; //Mr
    inline const size_t COLS_PER_REGISTER = 8; // nr
}