#include <iostream>
#include <iomanip>
#include <cstring>
#include <random>
#include <vector>
#include <cmath>
#include "monocypher.hpp"
#include "helpers.hpp"

using namespace std;
// 1. generate elligtor messages for the receiver.
// 2. decode the messages and retrieve the randomness for the intersection elements.
// 3. Send the polynomial.
// 4. Receiver computes the intersection.

class Receiver {
public:
    // Constructor
    Receiver(const uint256_t *input, size_t input_len) {
        this->input = vector<uint256_t>(input, input + input_len);
        this->input_len = input_len;

        ka_messages = vector<uint256_t>();
        polys = vector<std::vector<uint256_t>>();
        merkle_root_receiver = uint256_t();
    }

    uint256_t merkle_root_receiver;
private:
    vector<uint256_t> input;
    size_t input_len;
    vector<uint256_t> ka_messages;
    vector<std::vector<uint256_t>> polys;

    friend void commit_Receiver(Receiver &receiver);
    // friend void - intersect 1 and 2
};

class Sender {
public:
    // Constructor
    Sender(const uint256_t *input, size_t input_len) {
        this->input = vector<uint256_t>(input, input + input_len);
        this->input_len = input_len;

        random_values = vector<uint256_t>();
        merkle_root_sender = uint256_t();
        merkle_leaves = vector<uint256_t>();
    }

uint256_t merkle_root_sender;
private:
    vector<uint256_t> input;
    size_t input_len;
    vector<uint256_t> random_values;
    vector<uint256_t> merkle_leaves;

    friend void commit_Sender(Sender &sender);
    friend void intersect(Receiver &receiver, Sender &sender);
    
};

// Commitment phase for the receiver.
void commit_Receiver(Receiver &receiver){
    // 1. Generate KA messages.
    receiver.ka_messages.resize(receiver.input_len); // each KA message is 32 bytes
    receiver.ka_messages = gen_elligator_messages(receiver.ka_messages, receiver.input_len);
    // TODO: add Rijndael-256 on the ka messages
    // 2. Create uniform hashing table.
    uint64_t bin_size = receiver.input_len / log2(receiver.input_len); // n/log(n)
    std::vector<std::vector<uint256_t>> T_Rec(bin_size);
    // Hash each KA message and place it into the correct bin.
    for (const auto& current_message : receiver.ka_messages) {
        uint8_t hash[32];
        crypto_blake2b(hash, sizeof(hash), current_message.bytes, 32);
        size_t bin_index = H_bin(hash, bin_size);
        T_Rec[bin_index].push_back(current_message);
    }
    
    // 3. Polynomials using hashing table.
    // For each bin, create a polynomial with the elements in that bin.
    size_t ka_counter = 0;
    receiver.polys.clear();
    for (size_t i = 0; i < bin_size; i++) {
        const auto& bin_elements = T_Rec[i];
        if (bin_elements.empty()) {
            continue;
        }
        // TODO: add optimized lagrange (Manya)
        // Lagrange interpolation from ka_counter to ka_counter + bin_elements.size() - 1
        receiver.polys.push_back(Lagrange_Polynomial(bin_elements, receiver.ka_messages, ka_counter));
        ka_counter += bin_elements.size();
    }

    // 4. Merkle tree root using the evaluations at roots of unity.
    // TODO: add computer merkle tree (Manya)
    receiver.merkle_root_receiver = Merkle_Root_Receiver(receiver.polys);

}

void commit_Sender(Sender &sender){
    // 1. Generate input_len random field elements.
    for (size_t i = 0; i < sender.input_len; i++) {
        uint8_t random_value[32];
        random_device rd;
        for (size_t j = 0; j < 32; j++) {
            random_value[j] = rd() & 0xFF;
        }
        memcpy(&sender.random_values[i], random_value, 32);
    }

    // 2. Compute the Merkle root using the random values.
    sender.merkle_root_sender = Merkle_Root_Sender(sender.input,sender.random_values);

}

void intersect(Receiver &receiver, Sender &sender) {
    // 1. Receiver sends polynomials to the sender.

    // 2. Sender aborts if the Merkle root does not match.

    // 3. Sender 

}

int main() {

    return 0;
}