
#include <mcl/bn256.hpp>

#include <fstream>
#include <iostream>
#include <tuple>
static cybozu::RandomGenerator rg;
using namespace mcl::bn;

#include <mcl/lagrange.hpp>
#include <tuple>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <cstdlib>

#include <chrono>
#include <execution>
#include <algorithm>


using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;




using namespace std;


set<string> generate_credit_cards(int number_of_credit_cards)
{   
    set<string> credit_cards;
    
    int i, j;
    string str;
    char c;

    for(i=0; i<number_of_credit_cards; i++)
    {
        for (j=0; j<16; j++)
        {
            c = '0' + rand() % 10;
            str += c;
        }
        credit_cards.insert(str);
        str.clear();
    }
    return credit_cards;
}


// https://inversepalindrome.com/blog/how-to-create-a-random-string-in-cpp
string random_string(size_t length)
{
    const string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    random_device random_device;
    mt19937 generator(random_device());
    uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    string random_string;

    for (size_t i = 0; i < length; ++i)
    {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
}

int set_intersection_strings(set<string> xs, set<string> ys) {
    set<string> common_elements; 
    set_intersection(xs.begin(), xs.end(), ys.begin(), 
                     ys.end(), 
                     inserter(common_elements, common_elements.begin())); 

	return common_elements.size();
}




void Hash(G1& P, const string& m)
{
	Fp t;
	t.setHashOf(m);
	mapToG1(P, t);
}

void KeyGen(Fr& s, G2& pub, const G2& Q)
{
	s.setHashOf(random_string(32));
	G2::mul(pub, Q, s); // pub = sQ
}

void Sign(G1& sign, const Fr& s, const string& m)
{
	G1 Hm;
	Hash(Hm, m);
	G1::mul(sign, Hm, s); // sign = s H(m)
}

bool Verify(const G1& sign, const G2& Q, const G2& pub, const string& m)
{
	Fp12 e1, e2;
	G1 Hm;
	Hash(Hm, m);
	pairing(e1, sign, Q); // e1 = e(sign, Q)
	pairing(e2, Hm, pub); // e2 = e(Hm, sQ)
	return e1 == e2;
}


G1 judge_sign_apsi(string x, Fr sk, string client_id) {
	G1 sign;
	x = x + client_id;
	Sign(sign, sk, x);
	return sign;
}




Fp12 server_y_hats_apsi(string y, G2 pks, Fr s) {

	Fp12 y_hat;
	G1 Hy;
	Hash(Hy, y);
	pairing(y_hat, Hy, pks);
	return y_hat;

}




// Partial-APSI protocol, takes as input two sets of strings and returns their intersection size.
int PAPSI(set<string> xs, set<string> ys, int p)
{
	string client_id = "123456789";
	int authorize_time=0;
	int intersect_time=0;


	// First, we setup the parameters of the protocol
	initPairing();
	G2 Q;
	mapToG2(Q, 1);



	// The judge generates secret key and public key 
	auto t1 = high_resolution_clock::now();
	Fr sk;
	G2 pk;
	KeyGen(sk, pk, Q);


	// The authorization phase begins. 
	set<int> I;
	for (int i=0; i<xs.size(); i++) {
		if ( rand() % 100 < p) {
			I.insert(i);
		}
	}

	int secret = 42;


	auto t2 = high_resolution_clock::now();
	auto ms_int = duration_cast<milliseconds>(t2 - t1);
	authorize_time += ms_int.count();


	t1 = high_resolution_clock::now();
	Fr inv_r;
	const Fr r = rand();
	mcl::bn::Fr::inv(inv_r, r);
	vector<G1> blind_xs(xs.size());

	vector<string> new_xs;
	for (string x: xs) {
		new_xs.push_back(x);
	}
	#pragma omp parallel for 
	for (int i=0; i<new_xs.size(); i++) {
		string x = new_xs[i] + client_id;
		G1 Hx;
		Hash(Hx, x);
		G1 Hxr;
		G1::mul(Hxr, Hx, r); // This is X
		blind_xs[i] = Hxr;
	}

	const Fr random_eea = rand();
	int c_eea;
	vector<G1> tis(xs.size());
	#pragma omp parallel for 
	for (int i=0; i<new_xs.size(); i++) {
		if (I.contains(i)) {
			string x = new_xs[i] + client_id;
			G1 Hx;
			Hash(Hx, x);
			G1 Hxr;
			G1::mul(Hxr, Hx, random_eea); // This is t_i
			tis[i] = Hxr;
		}
	}

	
	hash<string> h;
	for (int i=0; i<new_xs.size(); i++) {
		string long_str = tis[i].serializeToHexStr().c_str();
		long_str += new_xs[i];
		c_eea = h(long_str+to_string(c_eea));
	}

	Fr c_tmp = c_eea;
	Fr s_eea = random_eea + c_tmp*r; 

	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	authorize_time += ms_int.count();

	t1 = high_resolution_clock::now();
	#pragma omp parallel for 
	for (int i=0; i<new_xs.size(); i++) {
		if (I.contains(i)) {
			string x_eea = new_xs[i] + client_id;
			G1 Hx_eea;
			Hash(Hx_eea, x_eea);

			G1 right; 
			G1::mul(right, Hx_eea, s_eea); 

			G1 check;
			G1::mul(check, blind_xs[i], c_tmp); 


			G1 left = tis[i] + check;
			bool correct;
			correct = left.serializeToHexStr()==right.serializeToHexStr();

			if (!correct) {
				cout << "Verification fails for item " << i << "\n";
			}
		}
	}

	vector<G1> signatures(blind_xs.size());
	#pragma omp parallel for 
	for (int i=0; i<blind_xs.size(); i++) {
		G1 sign;
		G1::mul(sign, blind_xs[i], sk); 
		signatures[i] = sign;
	}
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	authorize_time += ms_int.count();
	std::cout << "The authorization phase takes " <<  ms_int.count() << "ms\n";


	// The intersection phase begins: 
	t1 = high_resolution_clock::now();
	Fr s;
	s.setHashOf(random_string(32));
	G2 S;
	G2 pks;
	KeyGen(s, S, Q);
	G2::mul(pks, pk, s);	

	//The OPRF protocol follows: 
	Fr inv_t;
	const Fr t = rand();
	mcl::bn::Fr::inv(inv_t, t);
	int oprf_time = 0;
	vector<string> new_ys;
	for (string y: ys) {
		new_ys.push_back(y);
	}

	// OPRF Request 
	vector<G1> oprf_vector(new_ys.size());
	#pragma omp parallel for 
	for (int i=0; i<new_ys.size(); i++) {
		string y = new_ys[i] + client_id;
		G1 Hy;
		Hash(Hy, y);
		G1 Hyt;
		G1::mul(Hyt, Hy, t);
		oprf_vector[i] = Hyt;
	}

	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);

	intersect_time += ms_int.count();

	// OPRF Eval 
	t1 = high_resolution_clock::now();
	#pragma omp parallel for 
	for (int i=0; i<oprf_vector.size(); i++) {
		G1 Hytr;
		G1::mul(Hytr, oprf_vector[i], r);
		oprf_vector[i] = Hytr;
	}
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	intersect_time += ms_int.count();

	// OPRF Recover 
	t1 = high_resolution_clock::now();
	#pragma omp parallel for 
	for (int i=0; i<oprf_vector.size(); i++) {
		G1 Hytrinv_t;
		G1::mul(Hytrinv_t, oprf_vector[i], inv_t);
		oprf_vector[i] = Hytrinv_t;
	}
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	intersect_time += ms_int.count();


	// The server processes their elements
	t1 = high_resolution_clock::now();
	vector<string> y_hats(oprf_vector.size());

	#pragma omp parallel for 
	for (int i=0; i<oprf_vector.size(); i++) {
		Fp12 y_hat;
		pairing(y_hat, oprf_vector[i], pks);
		y_hats[i] = y_hat.getStr(256).c_str();
	}
	set<string> y_hats_set;
	for (string y: y_hats) {
		y_hats_set.insert(y);
	}

	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	intersect_time += ms_int.count();

	// The client now completes the intersection. 
	t1 = high_resolution_clock::now();
	vector<string> x_hats(signatures.size());
	#pragma omp parallel for 
	for (int i=0; i<signatures.size(); i++) {
		Fp12 x_hat;
		pairing(x_hat, signatures[i], S);
		x_hats[i] = x_hat.getStr(256).c_str();
	}
	set<string> x_hats_set;
	for (string x: x_hats) {
		x_hats_set.insert(x);
	}


	int inter_size = set_intersection_strings(x_hats_set,y_hats_set);
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	intersect_time += ms_int.count();
	std::cout << "The intersection phase takes " <<  intersect_time << "ms\n";
	return inter_size;
}




int main(int argc, char *argv[]) {
	std::string data_type = argv[1];
	int size_of_elements = atoi(argv[2]);
	int n = atoi(argv[3]);
	int m = atoi(argv[4]);
	srand (time(NULL));

	// p is an int from 1-100.
	int p = atoi(argv[5]);


	// We first generate the two datasets for the client and the server. 
	set<string> xs;
	if (data_type == "credit_cards") {
		xs = generate_credit_cards(n);
	} else {
		while (xs.size() < n) {
			xs.insert(random_string(size_of_elements));
		}
	}
	set<string> ys;
	if (data_type == "credit_cards") {
		ys = generate_credit_cards(m);
	} else {
		while (ys.size() < m) {
			ys.insert(random_string(size_of_elements));
		}
	}


	// Now, we conpute the ideal functionality (or the intersection of the two sets in plaintext)
    auto t1 = high_resolution_clock::now();
	int correct_inter = set_intersection_strings(xs,ys);
	auto t2 = high_resolution_clock::now();
	auto ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Ideal functionality requires " <<  ms_int.count() << "ms\n";

	
	t1 = high_resolution_clock::now();
	PAPSI(xs, ys,p);
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Partial-APSI protocol completed. Total time: " <<  ms_int.count() << "ms\n";




}