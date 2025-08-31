#include <iostream>
#include <iomanip>
#include <cstring>
#include <random>
#include <vector>
#include "monocypher.hpp"

using namespace std;

// Array of 32 elements of 8 bits each
struct uint256_t {
    uint8_t bytes[32];
};

// Function to generate Elligator messages.
// Input: pointer to a buffer to hold the KA messages, number of KA messages to generate
vector<uint256_t> gen_elligator_messages(vector<uint256_t> messages, size_t num_messages);

size_t hash_to_bin(const uint8_t hash[32], size_t bin_size);

vector<uint256_t> create_lagrange_polynomial(vector<uint256_t> poly, const vector<uint256_t> points, size_t start_index);

uint256_t compute_merkle_root(vector<vector<uint256_t>> leaves);


inline bool operator==(const uint256_t& a, const uint256_t& b) {
    return memcmp(a.bytes, b.bytes, 32) == 0;
}

namespace std {
    template <>
    struct hash<uint256_t> {
        size_t operator()(const uint256_t& k) const {
            uint8_t digest[32];
            // TODO: check if this can be optimized
            crypto_blake2b(digest, sizeof(digest), k.bytes, 32);
            size_t h;
            memcpy(&h, digest, sizeof(h));
            return h;
        }
    };
}