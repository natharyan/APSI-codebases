#include "simple_hashing.h"
#include <functional>

// TODO: replace
uint32_t hash_element(const Element& elem, uint32_t nbins, const hs_t* hs) {
    uint32_t sum = 0;
    for (auto b : elem) sum += b;
    return sum % nbins;
}

SimpleHashingContext simple_hashing(
    const std::vector<Element>& elements,
    uint32_t nbins,
    const std::shared_ptr<hs_t>& hs
) {
    // bin by hashes
    std::vector<std::vector<Element>> bins(nbins);
    for (const auto& elem : elements) {
        uint32_t i = hash_element(elem, nbins, hs.get());
        bins[i].push_back(elem);
    }

    // assign j values
    SimpleHashDict dict;
    for (uint32_t i = 0; i < nbins; ++i) {
        auto& bin = bins[i];
        std::sort(bin.begin(), bin.end());
        for (uint32_t j = 0; j < bin.size(); ++j) {
            dict[bin[j]] = BinPosition{i, j};
        }
    }

    return SimpleHashingContext{dict, nbins, hs};
}