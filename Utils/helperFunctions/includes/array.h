#pragma once
#include <cstddef> 
#include <algorithm>
namespace Utils {
template <typename Vec, typename Comp>
std::size_t repositionElement(Vec& v, std::size_t idx, Comp comp)
{
    if (idx >= v.size()) return idx;
    auto it = v.begin() + idx;

    // mora ka pocetku
    if (idx > 0 && comp(*it, *(it - 1))) {
        auto pos = std::upper_bound(v.begin(), it, *it, comp);
        std::rotate(pos, it, it + 1);
        return static_cast<std::size_t>(pos - v.begin());
    }

    // mora ka kraju
    if (idx + 1 < v.size() && comp(*(it + 1), *it)) {
        auto pos = std::lower_bound(it + 1, v.end(), *it, comp);
        std::rotate(it, it + 1, pos);
        return static_cast<std::size_t>(pos - v.begin()) - 1;
    }

    return idx;
}}