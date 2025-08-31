#include <iostream>
#include <iomanip>
#include <cstring>
#include <random>
#include <vector>
#include <cmath>
#include <unordered_set>
#include <algorithm> 
#include "monocypher.hpp"
#include "helpers.hpp"

using namespace std;

class Receiver {
public:
    // Constructor
    Receiver(const uint256_t *input, size_t input_len) {
        this->input = vector<uint256_t>(input, input + input_len);
        this->input_len = input_len;

        ka_messages = vector<uint256_t>();
        polys = vector<vector<uint256_t>>();
        merkle_root = uint256_t();
    }

    uint256_t merkle_root;
    vector<vector<uint256_t>> polys;
    size_t input_len; // TODO: make this private later
    vector<uint256_t> ka_messages;
    vector<uint256_t> randomness;
    vector<uint256_t> input;
// private:
//     friend void commit_Receiver(Receiver &receiver);
//     friend vector<uint256_t> intersect(Receiver &receiver, Sender &sender);
};

class Sender {
public:
    // Constructor
    Sender(const uint256_t *input, size_t input_len) {
        this->input = vector<uint256_t>(input, input + input_len);
        this->input_len = input_len;

        random_values = vector<uint256_t>();
        merkle_root = uint256_t();
        merkle_leaves = vector<uint256_t>();
    }

uint256_t merkle_root;
vector<uint256_t> merkle_leaves;
private:
    vector<uint256_t> input;
    size_t input_len;
    vector<uint256_t> random_values;

    friend void commit_Sender(Sender &sender);
    friend vector<uint256_t> intersect(Receiver &receiver, Sender &sender);
    
};

// Commitment phase for the receiver.
void commit_Receiver(Receiver &receiver){
    // 1. Generate KA messages.
    receiver.ka_messages.resize(receiver.input_len); // each KA message is 32 bytes
    tie(receiver.ka_messages, receiver.randomness) = gen_elligator_messages(receiver.input_len);
    // TODO: add Rijndael-256 on the ka messages
    // 2. Create uniform hashing table.
    uint64_t bin_size = receiver.input_len / log2(receiver.input_len); // n/log(n)
    vector<vector<uint256_t>> T_Rec(bin_size);
    // Hash each input message and place it into the correct bin.
    for (const auto& current_message : receiver.input) {
        uint8_t hash[32];
        crypto_blake2b(hash, sizeof(hash), current_message.bytes, 32);
        size_t bin_index = H_bin(hash, bin_size);
        T_Rec[bin_index].push_back(current_message);
    }
    
    // 3. Polynomials using hashing table.
    // For each bin, create a polynomial with the elements in that bin.
    size_t ka_counter = 0;
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
    receiver.merkle_root = Merkle_Root_Receiver(receiver.polys);

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
    // TODO: add sender.merkle_leaves = D' (Manya)
    sender.merkle_leaves = sender.random_values; // TODO: replace this with polynomial evaluations at roots of unity.
    sender.merkle_root = Merkle_Root_Sender(sender.input,sender.random_values);

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
    if (!(computed_root == receiver.merkle_root)) {
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
        // get the polynomial index using H_bin, then evaluate the polynomial using message
        uint8_t hash[32];
        crypto_blake2b(hash, sizeof(hash), message.bytes, 32);
        size_t bin_index = H_bin(hash, bin_size);
        // TODO: add implementation for evaluate_poly
        uint256_t g_b_i_sender = evaluate_poly(receiver.polys[bin_index], message.bytes);
        uint256_t g_b_i_a;
        crypto_x25519(g_b_i_a.bytes, a.bytes, g_b_i_sender.bytes);
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
        for (size_t j = 0; j < T_Sender[i].size(); j++) {
            r_values.push_back(sender.random_values[k_i_counter]);
            uint256_t k_i = k_values[k_i_counter];
            H_2_values.push_back(H_2(T_Sender[i][j], k_i));
            k_i_counter++;
        }
        P_Sender[i] = Lagrange_Polynomial(H_2_values, r_values);
    }
    printf("Sender sends %zu polynomials, m, and D' to the receiver.\n", P_Sender.size());
    // TODO: add communication cost here

    // 6. Receiver checks if D' corresponds to the published merkle root and computes the intersection
    if (!(sender.merkle_root == Merkle_Root_Sender(sender.input, sender.merkle_leaves))) {
        throw runtime_error("Receiver aborts: Merkle root does not match");
    }

    vector<uint256_t> R_intersection;
    for (size_t i = 0; i < receiver.input_len; i++) {
        // get the bin index using H_bin
        uint8_t hash[32];
        crypto_blake2b(hash, sizeof(hash), receiver.input[i].bytes, 32);
        size_t bin_index = H_bin(hash, bin_size);
        // compute H_2(x_i, k_i_receiver)
        uint256_t b_i = receiver.randomness[i];
        uint256_t g_b_i_a;
        crypto_x25519(g_b_i_a.bytes, b_i.bytes, m_sender.bytes); // g_b_i^a
        uint256_t k_i_receiver;
        crypto_blake2b(k_i_receiver.bytes, sizeof(k_i_receiver.bytes), g_b_i_a.bytes, sizeof(g_b_i_a.bytes)); // k_i_receiver = H(g_b_i^a)
        uint256_t r_i_receiver = evaluate_poly(P_Sender[bin_index], H_2(receiver.input[i], k_i_receiver).bytes);
        R_intersection.push_back(H_2(receiver.input[i],r_i_receiver)); // H_1 with concatentation is the same as H_2
    }

    unordered_set<uint256_t> merkle_set(sender.merkle_leaves.begin(), sender.merkle_leaves.end());
    vector<uint256_t> intersection;
    intersection.reserve(min(R_intersection.size(), sender.merkle_leaves.size())); // pre-allocation for efficiency
    for (const auto& item : R_intersection) {
        if (merkle_set.count(item)) {
            intersection.push_back(item);
        }
    }
    return intersection;
}

int main() {
    size_t receiver_size = 1000;
    size_t sender_size = 1000;
    vector<uint256_t> receiver_input(receiver_size);
    vector<uint256_t> sender_input(sender_size);
    random_device rd;
    for (size_t i = 0; i < receiver_size; i++) {
        for (size_t j = 0; j < 32; j++) {
            receiver_input[i].bytes[j] = rd() & 0xFF;
        }
    }
    for (size_t i = 0; i < sender_size; i++) {
        for (size_t j = 0; j < 32; j++) {
            sender_input[i].bytes[j] = rd() & 0xFF; 
        }
    }
    Receiver receiver(receiver_input.data(), receiver_size);
    Sender sender(sender_input.data(), sender_size);
    commit_Receiver(receiver);
    commit_Sender(sender);
    vector<uint256_t> intersection = intersect(receiver, sender);
    cout << "Intersection size: " << intersection.size() << endl;

    return 0;
}