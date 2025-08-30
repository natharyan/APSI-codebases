
// #include <mcl/bn256.hpp> 
#include <mcl/bls12_381.hpp> 

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
#include <stdio.h>
#include <string.h>
#include <gmp.h>
#include <math.h>
 #include <cmath>


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







void get_generator(mpz_t g, mpz_t p, mpz_t q, mpz_t n) {
    bool done = false;
    while (done == false) {
        mpz_t candidate;
        mpz_t candidate_modn;

        mpz_init(candidate);
        mpz_init(candidate_modn);
        mpz_set_ui(candidate, rand());
        mpz_mod(candidate_modn, candidate, n);




        mpz_t candidate_modp;
        mpz_t candidate_modq;
        mpz_init(candidate_modp);
        mpz_init(candidate_modq);

        mpz_t p_minus_one;
        mpz_t q_minus_one;

        mpz_init(p_minus_one);
        mpz_init(q_minus_one);

        mpz_mod(candidate_modp, candidate_modn, p);
        mpz_mod(candidate_modq, candidate_modn, q);

        mpz_sub_ui(p_minus_one, p, 1);
        mpz_sub_ui(q_minus_one, q, 1);

        if (mpz_cmp_ui(candidate_modp,1)!=0   and mpz_cmp_ui(candidate_modp,0)!=0 and mpz_cmp(candidate_modp,p_minus_one)!=0) {
            if (mpz_cmp_ui(candidate_modq,1)!=0   and mpz_cmp_ui(candidate_modq,0)!=0  and mpz_cmp(candidate_modq,q_minus_one)!=0) {
                mpz_set(g, candidate_modn);
                return;
            }
        }
    }
    return;
}


// DT10 protocol, takes as input two sets of strings and returns their intersection size
int DT10(set<string> xs, set<string> ys)
{
    int authorize_time=0;
    int intersect_time=0;

    vector<string> new_xs;
    for (string x: xs) {
        new_xs.push_back(x);
    }
    vector<string> new_ys;
    for (string y: ys) {
        new_ys.push_back(y);
    }

    // Generate judge's private-public key pair
    mpz_t n, e, d, p, q, g, n_minus_1, p_minus_one;

    mpz_init (n);
    mpz_set_str(n,"22633831336163704225802119544124700735796256790903029493256266492843673552534801592110030015200585858943006147796031263694221988644761804280063354687795635059525977427951187734798550093672407459247360581317849113849868884067110207646084113327381021219384754388530301255140565643838833506698383093690852531739531172609123678696740140273564830611806337982275991917029757927458885242870699437866762146339849706948716799290372936022693082524283082653328948087673985420969023252686920377008844234862256667849504500704103270750113135217281606318192401278553278120050590907173578032410734312613641534695934104780715101153383",10);
    mpz_init (n_minus_1);
    mpz_sub_ui(n_minus_1,n, 1);
    mpz_t n_over_four;
    mpz_init(n_over_four);
    mpz_t four;
    mpz_init(four);
    mpz_set_ui(four, 4);
    mpz_cdiv_q(n_over_four, n, four);

    mpz_init (e);
    mpz_set_ui(e,65537);
    mpz_init (d);
    mpz_set_str(d,"3613152012434879131030437381488817295541456559598814327150267178054084146461069231985826846194188462338247559672277935834245547480072294984177225334447990203896436758643595618985648276240913176352989706604625439508938893527474663051304331806934407189789024526798663529476182885482122711552988269013577384063157955435540991070745540479204276702813374945292793749402157720198742802270651593203633344223524591396690004485743985900286087535031993688962798116292473159035748265836390826138621837134718272093108163542429199339470795908835541516555605793685068642268841334044066423747085453648527698325673199461974022526641", 10);
    mpz_init (p);
    mpz_set_str(p,"2895406217824332604835806008182182634464067875091059425216275903532828551998417882090263498853251467656414959625616248592471161938518231405922759718837839755118385819795799062580650222904497738573890932692913671456947695140274605909140664100704828921363009859869257393666621867573025170693002177009747063002640378627325092965349",10);
    mpz_init (p_minus_one);
    mpz_sub_ui(p_minus_one, p, 1);
    mpz_init (q);
    mpz_set_str(q,"7817152286552464330441185065680670081119197665982952924760772371834619291452809688221089083626487837277939056315844084068438376895600483438474912967325565920437976199353376992248096513916369189003674378156314352430162122111821949059073751008889583844214703395563391099750954872622323775067",10);
    mpz_init(g);
    get_generator(g, p, q, n); 

    auto t1 = high_resolution_clock::now();
    #pragma omp parallel for 
    for (int i=0; i<new_xs.size(); i++) {
        t1 = high_resolution_clock::now();
        std::hash<std::string> hasher;
        auto hashed = hasher(new_xs[i]); 

        mpz_t hci; 
        mpz_init(hci);
        mpz_set_ui(hci,hashed);
        mpz_mod(hci, hci, p_minus_one); 
        mpz_add_ui(hci,hci,1);
        
        mpz_t sigma_i; 
        mpz_init(sigma_i);
        mpz_powm(sigma_i, hci, d, n);
    }
    auto t2 = high_resolution_clock::now();
    auto ms_int = duration_cast<milliseconds>(t2 - t1);
    authorize_time += ms_int.count();

    std::cout << "The authorization phase takes " <<  ms_int.count() << "ms\n";

    t1 = high_resolution_clock::now();
    mpz_t PCHstar;
    mpz_init(PCHstar);
    mpz_set_ui(PCHstar,1);
    std::hash<std::string> hasher;
    auto hashed = hasher(*xs.begin()); 
    mpz_t hci; 
    mpz_init(hci);
    mpz_set_ui(hci,hashed);
    mpz_mod(hci, hci, p_minus_one);
    mpz_add_ui(hci,hci,1);        
    mpz_t sigma_i; 
    mpz_init(sigma_i);
    mpz_powm(sigma_i, hci, d, n);

    for (int i=0; i<new_xs.size(); i++) {
        std::hash<std::string> hasher;
        auto hashed = hasher(new_xs[i]); 

        mpz_t hci; 
        mpz_init(hci);
        mpz_set_ui(hci,hashed);
        mpz_mod(hci, hci, p_minus_one); 
        mpz_add_ui(hci,hci,1);
        mpz_t sigma_i; 
        mpz_init(sigma_i);
        mpz_powm(sigma_i, hci, d, n);
        mpz_mul(PCHstar, PCHstar, sigma_i);

    }

    mpz_t Rc;
    mpz_init(Rc);
    mpz_set_ui(Rc, rand());
    mpz_mod(Rc,Rc,n_over_four);
    mpz_t X;
    mpz_init(X);
    mpz_mul(X, PCHstar, PCHstar);
    mpz_mod(X, X, n);
    mpz_t gRc; 
    mpz_init(gRc);
    mpz_powm(gRc,g, Rc, n);
    mpz_mul(X, X, gRc);
    mpz_mod(X,X,n);

    t2 = high_resolution_clock::now();
    ms_int = duration_cast<milliseconds>(t2 - t1);
    intersect_time += ms_int.count();
    
    t1 = high_resolution_clock::now();
    mpz_t Rs;
    mpz_init(Rs);
    mpz_set_ui(Rs, rand());
    mpz_mod(Rs,Rs,p_minus_one);
    mpz_add_ui(Rs, Rs, 1);
    mpz_t Z, eRs;
    mpz_init(Z);
    mpz_init(eRs);
    mpz_mul(eRs, Rs, e);
    mpz_powm(Z,g, eRs, n);

    t2 = high_resolution_clock::now();
    ms_int = duration_cast<milliseconds>(t2 - t1);
    intersect_time += ms_int.count();

    set<string> ti;
    set<string> ti1;

    vector<int> server_times(new_xs.size());
    vector<int> client_times(new_xs.size());

    #pragma omp parallel for 
    for (int i=0; i<new_xs.size(); i++) {

        t1 = high_resolution_clock::now();
        std::hash<std::string> hasher;
        auto hashed = hasher(new_xs[i]); 

        mpz_t hci; 
        mpz_init(hci);
        mpz_set_ui(hci,hashed);
        mpz_mod(hci, hci, p_minus_one); 
        mpz_add_ui(hci,hci,1);
        mpz_t sigma_i; 
        mpz_init(sigma_i);
        mpz_powm(sigma_i, hci, d, n);
        mpz_t PCHStar_i;
        mpz_init(PCHStar_i);
        mpz_cdiv_q(PCHStar_i, PCHstar, sigma_i);

        mpz_t Rci;
        mpz_init(Rci);
        mpz_set_ui(Rci, rand());
        mpz_mod(Rci,Rci,n_over_four);
        mpz_t y_i;
        mpz_init(y_i);
        mpz_mul(y_i, PCHStar_i, PCHStar_i);
        mpz_t gRci;
        mpz_init(gRci);
        mpz_powm(gRci,g, Rci, n);
        mpz_mul(y_i, y_i, gRci);

        t2 = high_resolution_clock::now();
        ms_int = duration_cast<milliseconds>(t2 - t1);
        client_times[i] = ms_int.count();

        t1 = high_resolution_clock::now();
        mpz_t y_i_1;
        mpz_init(y_i_1);
        mpz_powm(y_i_1, y_i, eRs, n);

        t2 = high_resolution_clock::now();
        ms_int = duration_cast<milliseconds>(t2 - t1);
        server_times[i] = ms_int.count();

        t1 = high_resolution_clock::now();
        mpz_t Kci;
        mpz_init(Kci);
        mpz_set(Kci,y_i_1);

        mpz_t ZRc;
        mpz_init(ZRc);
        mpz_powm(ZRc, Z, Rc, n);
        
        mpz_t Z_Rci;
        mpz_t minus_Rci;
        mpz_init(minus_Rci);
        mpz_init(Z_Rci);
        mpz_neg(minus_Rci,Rci);
        mpz_powm(Z_Rci, Z, minus_Rci, n);

        mpz_mul(Kci, Kci, ZRc);
        mpz_mul(Kci, Kci, Z_Rci);
        mpz_mod(Kci, Kci, n);

        std::hash<std::string> hasher2;
        auto hashed2 = hasher2(mpz_get_str(NULL,10,Kci)); 
        ti1.insert(to_string(hashed2));

        t2 = high_resolution_clock::now();
        ms_int = duration_cast<milliseconds>(t2 - t1);
        client_times[i] = ms_int.count();
    }

    for(std::vector<int>::iterator it = client_times.begin(); it != client_times.end(); ++it) {
        intersect_time += *it;
    }  

    for(std::vector<int>::iterator it = server_times.begin(); it != server_times.end(); ++it) {
        intersect_time += *it;
    }  

    t1 = high_resolution_clock::now();
    #pragma omp parallel for 
    for (int i=0; i<new_ys.size(); i++) {
        std::hash<std::string> hasher_hsj;
        auto hsj = hasher_hsj(new_ys[i]); 

        mpz_t Ksj;
        mpz_t Xe;
        mpz_t hsj2;
        mpz_t e_minus_2;
        mpz_t Xe_minus_2;
        mpz_init(Ksj);
        mpz_init(Xe);
        mpz_init(hsj2);
        mpz_init(e_minus_2);
        mpz_init(Xe_minus_2);
        mpz_sub_ui(e_minus_2,e,2);

        mpz_set_ui(hsj2, hsj);
        mpz_mod(hsj2, hsj2, p_minus_one);
        mpz_add_ui(hsj2,hsj2,1);
        mpz_mul(hsj2, hsj2, hsj2);
        mpz_powm(hsj2, hsj2, Rs, n);
        mpz_powm(Xe, X, eRs, n);
        mpz_invert(hsj2, hsj2, n);
        mpz_mul(Ksj, Xe, hsj2);
        mpz_mod(Ksj, Ksj, n);

        std::hash<std::string> hasher3;
        auto hashed3 = hasher3(mpz_get_str(NULL,10,Ksj)); 
        ti.insert(to_string(hashed3));
    }
    t2 = high_resolution_clock::now();
    ms_int = duration_cast<milliseconds>(t2 - t1);
    intersect_time += ms_int.count();

    t1 = high_resolution_clock::now();
    int set_inter_num = set_intersection_strings(ti, ti1);
    t2 = high_resolution_clock::now();
    ms_int = duration_cast<milliseconds>(t2 - t1);
    intersect_time += ms_int.count();

    std::cout << "The intersection phase takes " <<  intersect_time << "ms\n";

    return set_inter_num;


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

    
    // The DT10 APSI protocol follows: 
    t1 = high_resolution_clock::now();
    DT10(xs, ys);
    t2 = high_resolution_clock::now();
    ms_int = duration_cast<milliseconds>(t2 - t1);
    std::cout << "DT10 protocol completed. Total time: " <<  ms_int.count() << "ms\n";


}