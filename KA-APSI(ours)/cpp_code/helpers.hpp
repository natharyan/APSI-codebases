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

// Function to generate randomness and Elligator messages
// Input: number of KA messages to generate.
pair<vector<uint256_t>, vector<uint256_t>> gen_elligator_messages(size_t num_messages);

size_t H_bin(const uint8_t hash[32], size_t bin_size);

uint256_t H_2(const uint256_t& x_i, const uint256_t& k_i);

vector<uint256_t> Lagrange_Polynomial(vector<uint256_t> inputs, const vector<uint256_t> evaluations);

uint256_t Merkle_Root_Receiver(vector<vector<uint256_t>> leaves);

// Compute the Merkle root after appending input values with the ideal permutation of the random values
uint256_t Merkle_Root_Sender(vector<uint256_t> input, vector<uint256_t> random_values);


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