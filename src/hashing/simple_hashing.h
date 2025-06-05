#ifndef SIMPLE_HASHING_H_
#define SIMPLE_HASHING_H_

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>

struct hs_t {
    uint64_t seed1, seed2, seed3;

    uint32_t hash(const Element& elem, uint32_t nbins, int which) const;
};

using Element = std::vector<uint8_t>;

// (i, j) pair
struct BinPosition {
    uint32_t bin_index;
    uint32_t bin_offset;
};

// dictionary: element -> (i, j)
using SimpleHashDict = std::unordered_map<Element, BinPosition>;

struct SimpleHashingContext {
    SimpleHashDict table;
    uint32_t nbins;
    std::shared_ptr<hs_t> hs;
};

SimpleHashingContext simple_hashing(
    const std::vector<Element>& elements,
    uint32_t nbins,
    const std::shared_ptr<hs_t>& hs
);

#endif // SIMPLE_HASHING_H_