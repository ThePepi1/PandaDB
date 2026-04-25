// src/config/Config.cpp

#include <fstream>
#include <string>
#include <algorithm>
#include "CASH_SIZE.h"
namespace config {

    // ── Cache size detection ────────────────────────────────────────────────

    static size_t readCacheSize(const std::string& path) {
        std::ifstream f(path);
        size_t value = 0;
        if (f.is_open()) f >> value;
        return value;
    }

    size_t getL1Size() {
        static const size_t v = []() -> size_t {
            size_t s = readCacheSize(
                "/sys/devices/system/cpu/cpu0/cache/index1/size"); // index1 = L1d
            return s ? s : (32 * 1024);
        }();
        return v;
    }

    size_t getL2Size() {
        static const size_t v = []() -> size_t {
            size_t s = readCacheSize(
                "/sys/devices/system/cpu/cpu0/cache/index2/size");
            return s ? s : (256 * 1024);
        }();
        return v;
    }

    size_t getL3Size() {
        static const size_t v = []() -> size_t {
            size_t s = readCacheSize(
                "/sys/devices/system/cpu/cpu0/cache/index3/size");
            return s ? s : (8 * 1024 * 1024);
        }();
        return v;
    }

    // ── Helper: round x down to the nearest multiple of factor ─────────────
    static constexpr size_t alignDown(size_t x, size_t factor) {
        return (x / factor) * factor;
    }

    // ── Tuning constants derived from actual CPU cache sizes ────────────────
    //
    // Working-set targets:
    //   B panel  (Kc × Nc) × 8 bytes  ──► fits in L1d
    //   A panel  (Mc × Kc) × 8 bytes  ──► fits in L2
    //   micro-kernel registers: Mr=4, Nr=8  (fixed in header)
    //
    // We use ~50 % of each cache level to leave room for the OS, stack, etc.

    static const size_t ELEM = sizeof(double);  // 8 bytes

    // Kc: depth of the shared cache block.
    // Target: Kc × Nr × ELEM ≤ 0.5 × L1  →  Kc = L1 / (2 × Nr × ELEM)
    const size_t SHARED_PER_CACHE_BLOCK = []() -> size_t {
        const size_t kc = getL1Size() / (2 * config::COLS_PER_REGISTER * ELEM);
        // Round down to a multiple of 8 for SIMD alignment
        return std::max(alignDown(kc, 8), size_t(8));
    }();

    // Nc: columns of B kept in L1.
    // Target: Kc × Nc × ELEM ≤ 0.5 × L1  →  Nc = L1 / (2 × Kc × ELEM)
    const size_t COLS_PER_L1_BLOCK = []() -> size_t {
        const size_t nc = getL1Size() / (2 * SHARED_PER_CACHE_BLOCK * ELEM);
        // Must be a multiple of Nr=8
        return std::max(alignDown(nc, config::COLS_PER_REGISTER), config::COLS_PER_REGISTER);
    }();

    // Mc: rows of A kept in L2.
    // Target: Mc × Kc × ELEM ≤ 0.5 × L2  →  Mc = L2 / (2 × Kc × ELEM)
    const size_t ROWS_PER_L2_BLOCK = []() -> size_t {
        const size_t mc = getL2Size() / (2 * SHARED_PER_CACHE_BLOCK * ELEM);
        // Must be a multiple of Mr=4
        return std::max(alignDown(mc, config::ROWS_PER_REGISTER), config::ROWS_PER_REGISTER);
    }();

    // General-purpose square block: geometric mean of Mc and Nc, aligned to 8
    const size_t OPTIMAL_BLOCK = []() -> size_t {
        const size_t side = static_cast<size_t>(
            __builtin_sqrt(static_cast<double>(ROWS_PER_L2_BLOCK) * COLS_PER_L1_BLOCK));
        return std::max(alignDown(side, 8), size_t(8));
    }();

} // namespace config