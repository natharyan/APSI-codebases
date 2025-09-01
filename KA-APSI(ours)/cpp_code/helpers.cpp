#include <iostream>
#include <iomanip>
#include <cstring>
#include <random>
#include <vector>
#include <sstream>
#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include "monocypher.hpp"
#include "helpers.hpp"

using namespace std;
using namespace NTL;

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

ZZ bytes_to_ZZ(const uint256_t& num) {
    ZZ result = ZZ(0);
    ZZ base = ZZ(1);
    
    for (int i = 0; i < 32; i++) {
        result += ZZ(num.bytes[i]) * base;
        base *= 256;
    }
    
    return result;
}

uint256_t ZZ_to_bytes(const ZZ& num) {
    ZZ prime = conv<ZZ>("115792089237316195423570985008687907853269984665640564039457584007913129639937");
    uint256_t result;
    memset(result.bytes, 0, 32);

    ZZ temp = num % prime;
    if (temp < 0) temp += prime;

    for (int i = 0; i < 32 && temp > 0; i++) {
        result.bytes[i] = to_long(temp % 256);
        temp /= 256;
    }

    return result;
}


vector<uint256_t> Lagrange_Polynomial(vector<uint256_t> inputs,
                                      const vector<uint256_t> evaluations) {
    size_t n = inputs.size();
    if (n == 0 || n != evaluations.size()) {
        throw runtime_error("Invalid input for Lagrange interpolation");
    }
    
    if (n < 2) {
        throw runtime_error("Need at least 2 points for meaningful interpolation");
    }
    
    // Prime field
    ZZ prime = conv<ZZ>("115792089237316195423570985008687907853269984665640564039457584007913129639937");
    ZZ_p::init(prime);
    
    cout << "Lagrange_Polynomial: Interpolating " << n << " points:" << endl;
    for (size_t i = 0; i < min(n, 3UL); i++) {
        cout << "  Point " << i << ": input=" << bytes_to_ZZ(inputs[i]) 
             << ", eval=" << bytes_to_ZZ(evaluations[i]) << endl;
    }
    
    vec_ZZ_p zp_inputs, zp_evals;
    zp_inputs.SetLength(n);
    zp_evals.SetLength(n);
    
    for (size_t i = 0; i < n; i++) {
        ZZ temp_input = bytes_to_ZZ(inputs[i]) % prime;
        if (temp_input < 0) temp_input += prime;
        zp_inputs[i] = to_ZZ_p(temp_input);
        
        ZZ temp_eval = bytes_to_ZZ(evaluations[i]) % prime;
        if (temp_eval < 0) temp_eval += prime;
        zp_evals[i] = to_ZZ_p(temp_eval);
    }
    
    //check for duplicate x-coordinates
for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++) {
        if (zp_inputs[i] == zp_inputs[j]) {
            throw runtime_error(
                "Duplicate x-coordinate detected at positions " + 
                to_string(i) + " and " + to_string(j)
            );
        }
    }
}
    //using ntl's interpolation
    ZZ_pX P;
    interpolate(P, zp_inputs, zp_evals);
    
    long degree = deg(P);
    cout << "Polynomial degree: " << degree << endl;
    
    if (degree < 0) {
        vector<uint256_t> result(2);
        result[0] = evaluations[0]; 
        memset(result[1].bytes, 0, 32);
        cout << "Created fallback degree-1 polynomial" << endl;
        return result;
    }
    
    vector<uint256_t> result(degree + 1);
    
    for (long i = 0; i <= degree; i++) {
        ZZ coeff_zz = rep(coeff(P, i));
        result[i] = ZZ_to_bytes(coeff_zz);
    }
    
    cout << "Generated polynomial with " << result.size() << " coefficients" << endl;
    return result;
}
void test_interpolation_result(const vector<uint256_t>& coeffs,
                              const vector<uint256_t>& x,
                              const vector<uint256_t>& y) {
    cout << "Testing result polynomial" << endl;

    ZZ prime = conv<ZZ>("115792089237316195423570985008687907853269984665640564039457584007913129639937");
    ZZ_p::init(prime);

    ZZ_pX P;
    for (size_t j = 0; j < coeffs.size(); j++) {
        ZZ temp_coeff = bytes_to_ZZ(coeffs[j]);
        SetCoeff(P, j, to_ZZ_p(temp_coeff));         
    }

    for (size_t i = 0; i < x.size(); i++) {
        ZZ temp_x = bytes_to_ZZ(x[i]);
        ZZ_p zp_x = to_ZZ_p(temp_x);

        ZZ_p res = eval(P, zp_x);                    
        ZZ result_zz = rep(res);
        uint256_t result = ZZ_to_bytes(result_zz);

        if (memcmp(result.bytes, y[i].bytes, 32) != 0){
            cout << "Error! x = " << bytes_to_ZZ(x[i]) 
                << ", expected y = " << bytes_to_ZZ(y[i]) 
                << ", got y = " << bytes_to_ZZ(result) << endl;
            return;
        }
    }

    cout << "Polynomial is interpolated correctly!" << endl;
}

//helper function to combine two hashes for Merkle tree
uint256_t combine_hashes(const uint256_t& left, const uint256_t& right) {
    uint256_t result;
    uint8_t combined[64];
    memcpy(combined, left.bytes, 32);
    memcpy(combined + 32, right.bytes, 32);
    crypto_blake2b(result.bytes, 32, combined, 64);
    return result;
}

//to convert bytes to field element
uint256_t bytes_to_field(const uint8_t* bytes) {
    uint256_t result;
    memcpy(result.bytes, bytes, 32);
    return result;
}


uint256_t Merkle_Root_Receiver(vector<vector<uint256_t>> leaves) {
    if (leaves.empty()) {
        uint256_t zero;
        memset(zero.bytes, 0, 32);
        return zero;
    }
    
    vector<uint256_t> merkle_leaves;
    
    //creating leaves by hashing each polynomial
    for (const auto& poly : leaves) {
        uint256_t poly_hash;
        if (poly.empty()) {
            //hash of empty polynomial
            crypto_blake2b(poly_hash.bytes, 32, nullptr, 0);
        } else {
            //hashing all coefficients together
            vector<uint8_t> poly_bytes;
            for (const auto& coeff : poly) {
                poly_bytes.insert(poly_bytes.end(), coeff.bytes, coeff.bytes + 32);
            }
            crypto_blake2b(poly_hash.bytes, 32, poly_bytes.data(), poly_bytes.size());
        }
        merkle_leaves.push_back(poly_hash);
    }
    
    //building Merkle tree bottom-up
    vector<uint256_t> current_level = merkle_leaves;
    
    while (current_level.size() > 1) {
        vector<uint256_t> next_level;
        
        for (size_t i = 0; i < current_level.size(); i += 2) {
            if (i + 1 < current_level.size()) {
                //pair exists
                next_level.push_back(combine_hashes(current_level[i], current_level[i + 1]));
            } else {
                //odd number of nodes, hash with itself
                next_level.push_back(combine_hashes(current_level[i], current_level[i]));
            }
        }
        
        current_level = next_level;
    }
    
    return current_level[0];
}

uint256_t Merkle_Root_Sender(vector<uint256_t> input, vector<uint256_t> random_values) {
    if (input.size() != random_values.size() || input.empty()) {
        uint256_t zero;
        memset(zero.bytes, 0, 32);
        return zero;
    }
    
    vector<uint256_t> leaves;

    //creating leaves by combining input and random values
    for (size_t i = 0; i < input.size(); i++) {
        uint256_t leaf_hash;
        uint8_t combined[64];
        memcpy(combined, input[i].bytes, 32);
        memcpy(combined + 32, random_values[i].bytes, 32);
        crypto_blake2b(leaf_hash.bytes, 32, combined, 64);
        leaves.push_back(leaf_hash);
    }
    
    vector<uint256_t> current_level = leaves;
    
    while (current_level.size() > 1) {
        vector<uint256_t> next_level;
        
        for (size_t i = 0; i < current_level.size(); i += 2) {
            if (i + 1 < current_level.size()) {
                next_level.push_back(combine_hashes(current_level[i], current_level[i + 1]));
            } else {
                next_level.push_back(combine_hashes(current_level[i], current_level[i]));
            }
        }
        
        current_level = next_level;
    }
    
    return current_level[0];
}

uint256_t evaluate_poly(const vector<uint256_t>& poly, const uint8_t* point_bytes) {
    if (poly.empty()) {
        uint256_t zero;
        memset(zero.bytes, 0, 32);
        return zero;
    }
    
    //same prime as lagrange 
    ZZ prime = conv<ZZ>("115792089237316195423570985008687907853269984665640564039457584007913129639937");
    ZZ_p::init(prime);
    
    //reconstructing polynomial from coefficients
    ZZ_pX P;
    for (size_t i = 0; i < poly.size(); i++) {
        ZZ coeff_zz = bytes_to_ZZ(poly[i]) % prime;
        if (coeff_zz < 0) coeff_zz += prime;
        SetCoeff(P, i, to_ZZ_p(coeff_zz));
    }
    
    //convert point_bytes to uint256_t first, then to ZZ
    uint256_t point;
    memcpy(point.bytes, point_bytes, 32);
    
    ZZ point_zz = bytes_to_ZZ(point) % prime;
    if (point_zz < 0) point_zz += prime;
    ZZ_p point_zp = to_ZZ_p(point_zz);
    
    //evaluate polynomial at the point
    ZZ_p result_zp = eval(P, point_zp);
    ZZ result_zz = rep(result_zp);
    
    return ZZ_to_bytes(result_zz);
}
