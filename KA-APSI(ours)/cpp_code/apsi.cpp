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
        polys = vector<vector<uint256_t>>();
        merkle_root_receiver = uint256_t();
    }

    uint256_t merkle_root_receiver;
    vector<vector<uint256_t>> polys;
    size_t input_len; // TODO: make this private later
private:
    vector<uint256_t> input;
    vector<uint256_t> ka_messages;

    friend void commit_Receiver(Receiver &receiver);
    friend vector<uint256_t> intersect(Receiver &receiver, Sender &sender);
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
    friend vector<uint256_t> intersect(Receiver &receiver, Sender &sender);
    
};

// Commitment phase for the receiver.
void commit_Receiver(Receiver &receiver){
    // 1. Generate KA messages.
    receiver.ka_messages.resize(receiver.input_len); // each KA message is 32 bytes
    receiver.ka_messages = gen_elligator_messages(receiver.ka_messages, receiver.input_len);
    // TODO: add Rijndael-256 on the ka messages
    // 2. Create uniform hashing table.
    uint64_t bin_size = receiver.input_len / log2(receiver.input_len); // n/log(n)
    vector<vector<uint256_t>> T_Rec(bin_size);
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
        vector<uint256_t> ka_messages;
        for (size_t j = 0; j < bin_elements.size(); j++) {
            ka_messages.push_back(receiver.ka_messages[ka_counter]);
            ka_counter++;
        }
        receiver.polys.push_back(Lagrange_Polynomial(bin_elements, ka_messages));
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

    // 2. Compute the Merkle root using the random values
    sender.merkle_root_sender = Merkle_Root_Sender(sender.input,sender.random_values);

}

vector<uint256_t> intersect(Receiver &receiver, Sender &sender) {
    // 1. Receiver sends polynomials to the sender
    printf("Receiver sends %zu polynomials to the sender.\n", receiver.polys.size());
    // TODO: add exchange cost here

    // 2. Sender aborts if any(deg(receiver's poly)) < 1 or the Merkle root does not match
    for (const auto& poly : receiver.polys) {
        if (poly.size() < 2) {
            throw runtime_error("Sender aborts: Polynomial degree < 1");
        }
    }
    // check if merkle root created using receiver.polys matches with receiver.merkle_root_receiver
    uint256_t computed_root = Merkle_Root_Receiver(receiver.polys);
    if (!(computed_root == receiver.merkle_root_receiver)) {
        throw runtime_error("Sender aborts: Merkle root does not match");
    }

    // 3. Sender computes a random value and its ka message (the sender gets the total elements in the receiver's input using the number of polynomials and their degrees)
    size_t num_receiver_elements = 0;
    for (const auto& poly : receiver.polys) {
        num_receiver_elements += poly.size(); // degree + 1 = no. of elements
    }
    if (num_receiver_elements != receiver.input_len) {
        throw runtime_error("Sender aborts: Number of receiver elements does not match");
    }
    // Compute a and m = g^a using X25519 without elligator encoding
    uint256_t a = uint256_t();
    random_device rd;
    for (size_t j = 0; j < 32; j++) {
        a.bytes[j] = rd() & 0xFF;
    }
    uint8_t g_a[32];
    crypto_x25519_public_key(g_a, a.bytes);
    uint256_t m_sender = uint256_t();
    memcpy(m_sender.bytes, g_a, 32);

    // 4. Sender uses the m_sender to get k_i for each of its inputs
    uint64_t bin_size = num_receiver_elements / log2(num_receiver_elements); // n/log(n)
    vector<vector<uint256_t>> T_Sender(bin_size);
    vector<uint256_t> k_values(sender.input_len);
    for(auto& message: sender.input){
        // get the polynomial index using H_bin, then evaluate the polynomial at that index
        uint8_t hash[32];
        crypto_blake2b(hash, sizeof(hash), message.bytes, 32);
        size_t bin_index = H_bin(hash, bin_size);
        // TODO: add implementation for evaluate_poly
        uint256_t g_b_i = evaluate_poly(receiver.polys[bin_index], message.bytes);
        uint256_t g_b_i_a;
        crypto_x25519(g_b_i_a.bytes, a.bytes, g_b_i.bytes);
        uint256_t k_i;
        crypto_blake2b(k_i.bytes, sizeof(k_i.bytes), g_b_i_a.bytes, sizeof(g_b_i_a.bytes)); // k_i = H(g_b_i^a)
        k_values.push_back(k_i);
        T_Sender[bin_index].push_back(message);
    }

    // 5. Sender computes P_{j} for each bin and sends these to the receiver
    vector<vector<uint256_t>> P_Sender(bin_size);
    size_t k_i_counter = 0;
    for (size_t i = 0; i < bin_size; i++) {
        vector<uint256_t> H_2_values;
        vector<uint256_t> r_values;
        vector<uint256_t> k_values_bin;
        for (size_t j = 0; j < T_Sender[i].size(); j++) {
            r_values.push_back(k_values[k_i_counter]);
            uint256_t k_i = k_values[k_i_counter];
            H_2_values.push_back(H_2(T_Sender[i][j], k_i));
            k_values_bin.push_back(k_i);
            k_i_counter++;
        }
        P_Sender[i] = Lagrange_Polynomial(H_2_values, r_values);
    }
    printf("Sender sends %zu polynomials and m to the receiver.\n", P_Sender.size());
    // TODO: add communication cost here

    // 6. Receiver computes the intersection
        

}

int main() {

    return 0;
}