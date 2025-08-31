#include <iostream>
#include <iomanip>
#include <cstring>
#include <random>
#include "monocypher.hpp"
#include "helpers.hpp"

using namespace std;

pair<vector<uint256_t>, vector<uint256_t>> gen_elligator_messages(size_t num_messages) {
    vector<uint256_t> messages(num_messages);
    vector<uint256_t> randomness(num_messages);
    for (size_t i = 0; i < num_messages; i++) {
        uint8_t b_i[32];
        random_device rd;
        for (size_t j = 0; j < 32; j++) {
            b_i[j] = rd() & 0xFF;
        }
        // Compute g^b using X25519
        uint8_t g_b[32];
        crypto_x25519_public_key(g_b, b_i);
        // Elligator encoding
        uint8_t encoded[32];
        crypto_elligator_map(encoded, g_b);

        memcpy(&messages[i], encoded, 32);
        memcpy(&randomness[i], b_i, 32);
    }
    return make_pair(messages, randomness);
}

// Hash elements to bin indices in [0,n/log(n)-1], where n is the input size of the receiver
size_t H_bin(const uint8_t hash[32], size_t bin_size) {
    uint64_t bin_index;
    memcpy(&bin_index, hash, sizeof(bin_index));
    return bin_index % bin_size;
}

uint256_t H_2(const uint256_t& x_i, const uint256_t& k_i) {
    // Concatenate x_i and k_i
    uint8_t input[64];
    memcpy(input, x_i.bytes, 32);
    memcpy(input + 32, k_i.bytes, 32);
    
    // Hash the concatenation
    uint256_t result;
    crypto_blake2b(result.bytes, sizeof(result.bytes), input, sizeof(input));
    return result;
}
