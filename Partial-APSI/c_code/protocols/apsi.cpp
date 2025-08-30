
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

#include <atomic>
#include <thread>
#include <cmath>

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;




using namespace std;

struct NetworkSimulator {
    // latency in ms, bandwidth in kilobits per second (kbps)
    long latency_ms_client_to_server = 1;
    long latency_ms_server_to_client = 1;
    long bandwidth_kbps = 50000;

    std::atomic<size_t> bytes_client_to_server{0};
    std::atomic<size_t> bytes_server_to_client{0};

    NetworkSimulator() = default;
    NetworkSimulator(long lcs, long lsc, long bw)
        : latency_ms_client_to_server(lcs),
          latency_ms_server_to_client(lsc),
          bandwidth_kbps(bw) {}

    // compute transmission delay in ms given bytes and bandwidth (kbps).
    static long transmit_ms_for_bytes(size_t bytes, long kbps) {
        if (kbps <= 0) return 0;
        double ms = (double)bytes * 8.0 / (double)kbps;
        return (long)std::ceil(ms);
    }

    // simulate sending from client to server: blocks for latency+transmit and increments counters.
    void sendClientToServer(const std::string &msg) {
        size_t bytes = msg.size();
        bytes_client_to_server += bytes;
        long ttx = transmit_ms_for_bytes(bytes, bandwidth_kbps);
        long total = latency_ms_client_to_server + ttx;
        std::this_thread::sleep_for(std::chrono::milliseconds(total));
    }

    // simulate sending from server to client
    void sendServerToClient(const std::string &msg) {
        size_t bytes = msg.size();
        bytes_server_to_client += bytes;
        long ttx = transmit_ms_for_bytes(bytes, bandwidth_kbps);
        long total = latency_ms_server_to_client + ttx;
        std::this_thread::sleep_for(std::chrono::milliseconds(total));
    }

    // helpers to read totals (in bytes)
    size_t totalSentBytes() const { return bytes_client_to_server + bytes_server_to_client; }
    size_t totalClientToServer() const { return bytes_client_to_server.load(); }
    size_t totalServerToClient() const { return bytes_server_to_client.load(); }
};


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





// APSI protocol, takes as input two sets of strings and returns their intersection size.
int APSI(set<string> xs, set<string> ys, NetworkSimulator &net)
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

    // Client sends its set to the judge for signing
    printf("Client sending %zu elements to Judge for signing\n", xs.size());
    vector<string> new_xs;
    for (const string &x : xs) {
        new_xs.push_back(x);
        net.sendClientToServer(x); // emulate sending client set to judge
    }

	// Judge signs all the client elements
	// vector<string> new_xs;
	// for (string x: xs) {
	// 	new_xs.push_back(x);
	// }
    printf("Judge signing %zu elements for Client and sending to the Client\n", new_xs.size());
	vector<G1> signatures(new_xs.size());
    // Create a vector of serialized signatures to "send" over network
    vector<string> serialized_sigs(new_xs.size());

	#pragma omp parallel for 
	for (int i=0; i<new_xs.size(); i++) {
		G1 sign = judge_sign_apsi(new_xs[i], sk, client_id);
		signatures[i] = sign;
        // serialize signature to string form for sending
        std::string s = sign.getStr(256).c_str();
        serialized_sigs[i] = s;
	}

    // simulate sending signatures from Judge to Client (using the same latency)
    for (const auto &s : serialized_sigs) {
        net.sendServerToClient(s);
    }

	auto t2 = high_resolution_clock::now();
	auto ms_int = duration_cast<milliseconds>(t2 - t1);
	printf("The authorization phase takes %ldms\n", ms_int.count());
	authorize_time += ms_int.count();

	// Beginning of the intersection phase. 
	// The server processes their elements and sends them to the client
    printf("Server sending %zu elements to Client for intersection after processing\n", ys.size());
	t1 = high_resolution_clock::now();
	Fr s;
	G2 S;
	KeyGen(s, S, Q);
	G2 pks;
	G2::mul(pks, pk, s);

	vector<string> new_ys;
	for (string y: ys) {
		new_ys.push_back(y);
	}

	#pragma omp parallel for 
	for (int i=0; i<new_ys.size(); i++) {
		string y_c = new_ys[i] + client_id;
		Fp12 y_hat = server_y_hats_apsi(y_c, pks, s);
		new_ys[i] = y_hat.getStr(256).c_str();
	}

    // Simulate sending server's processed elements to client
	set<string> y_hats_set;
	for (string y: new_ys) {
        net.sendServerToClient(y);
		y_hats_set.insert(y);
	}

	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	intersect_time += ms_int.count();

	// The client completes the intersection protocol
    printf("Client computing the intersection\n");
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

    size_t sent_kb = (net.totalClientToServer() + net.totalServerToClient()) / 1024;
    double sent_kb_f = (double)(net.totalClientToServer() + net.totalServerToClient()) / 1024.0;
    printf("Total Comm = %.2f KB\n", sent_kb_f);
    printf("client->server+judge bytes: %ld\n", net.totalClientToServer());
    printf("server->client bytes: %ld\n", net.totalServerToClient());

	return inter_size;

}



int main(int argc, char *argv[]) {
	std::string data_type = argv[1];
	int size_of_elements = atoi(argv[2]);
	int n = atoi(argv[3]);
	int m = atoi(argv[4]);
	srand (time(NULL));

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
	set_intersection_strings(xs,ys);
	auto t2 = high_resolution_clock::now();
	auto ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Ideal functionality requires " <<  ms_int.count() << "ms\n";

	
	// default: LAN-like
    string netmode = "lan";
    long lat_cs = 1, lat_sc = 1, bw_kbps = 100000; // defaults (fast LAN)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) {
            netmode = argv[++i];
        }
    }
    if (netmode == "wan") {
        lat_cs = 40;  // 80 ms round-trip time
        lat_sc = 40;
        bw_kbps = 50000; // 50 Mbps
        // bw_kbps = 1000; // 1 Mbps
    } else if (netmode == "lan") {
        lat_cs = 0.1;
        lat_sc = 0.1;
        bw_kbps = 10000000; // 10 Gbps
    }

    printf("Network mode: %s\n", netmode.c_str());
    NetworkSimulator net(lat_cs, lat_sc, bw_kbps);

    // The APSI protocol follows:
    t1 = high_resolution_clock::now();
    int intersection = APSI(xs, ys, net);
    t2 = high_resolution_clock::now();
    ms_int = duration_cast<milliseconds>(t2 - t1);
    std::cout << "APSI protocol completed. Total time: " <<  ms_int.count() << "ms\n";

}