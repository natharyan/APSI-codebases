#include <iostream>
#include <iomanip>
#include <cstring>
#include <random>
#include "monocypher.hpp"
#include "helpers.hpp"

using namespace std;

// Function to generate Elligator messages.
// Input: pointer to a buffer to hold the KA messages, number of KA messages to generate.
vector<uint256_t> gen_elligator_messages(vector<uint256_t> messages, size_t num_messages) {
    for (size_t i = 0; i < num_messages; i++) {
        uint8_t scalar[32];
        random_device rd;
        for (size_t j = 0; j < 32; j++) {
            scalar[j] = rd() & 0xFF;
        }
        // Compute g^b using X25519
        uint8_t g_b[32];
        crypto_x25519_public_key(g_b, scalar);
        // Elligator encoding
        uint8_t encoded[32];
        crypto_elligator_map(encoded, g_b);

        memcpy(&messages[i], encoded, 32);
    }
    return messages;
}

// Hash elements to bin indices in [0,n/log(n)-1], where n is the input size of the receiver
size_t H_bin(const uint8_t hash[32], size_t bin_size) {
    uint64_t bin_index;
    memcpy(&bin_index, hash, sizeof(bin_index));
    return bin_index % bin_size;
}