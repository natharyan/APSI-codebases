This is the artifact for "Re-visiting Authorized Private Set Intersection: A New Privacy-Preserving Variant and Two Protocols" by Francesca Falzon and Evangelia Anna Markatou (PoPETS 2025). This respository builds upon the [MCL Pairings Library](https://github.com/herumi/mcl/tree/master). 

In this respository, you will find implementations and tests for three protocols:

1. The Authorized Private Set Intersection (APSI) protocol from Falzon and Markatou (PoPETS 2025) [[code](c_code/protocols/apsi.cpp)]
2. The Partial-APSI protocol from Falzon and Markatou (PoPETS 2025) [[code](c_code/protocols/papsi.cpp)]
3. The APSI protocol from De Cristofaro and Tsudik (Financial Crypto 2010) [[code](c_code/protocols/dt10.cpp)]

The third protocol is re-implemented for comparison. 

# Getting Started

First make sure that the [GMP Library](https://gmplib.org/), [LLVM](https://llvm.org/), and [libOMP](https://formulae.brew.sh/formula/libomp) are installed.
For example, for MacOS, they can be installed using the following terminal command:

    brew install gmp llvm libomp

Clone this respository and navigate to the `c_code` directory. There are three Makefiles: `Makefile-Linux`, `Makefile-MacM1`, and `Makefile-MacM2`.
Find the correct one for your machine and rename it to `Makefile`. If you want to run on an M2 Mac, there is an additional change. Replace `common.mk` with `common-m2.mk`. 

Then run:

    make clean
    make protocols

# Running the Protocols

To run our APSI, Partial-APSI or DT10's protocol (from "Practical Private Set Intersection Protocols with Linear Computational and Bandwidth Complexity" by Emiliano De Cristofaro and Gene Tsudik (FC 2010)), use the following command:

    bin/{apsi, papsi, dt10}.exe {credit_cards, random} lenght_of_random_string n m (p)

Here, `credit_cards` generates random 16 digit strings, and `random` generates random strings of length specified by `length_of_random_string` (note that the `length_of_random_string` parameter must be added regardless of whether the credit card or random option is chosen; however, in the case of the credit card option, the parameter is simply ignored). Parameters `n` and `m` denote the sizes of the client and server sets, respectively. The `outputfile` is the name of the file to which the results are written to. Laslty, `p` is a percent from 1 to 100 denoting the percent of client values revealed to judge in Partial-APSI (this parameter is to be omitted when running APSI and DT10).

Example for APSI with a random dataset:

    bin/apsi.exe random 4 1000 1000 

Example for Partial-APSI with a credit card dataset:

    bin/papsi.exe credit_cards 0 1000 1000  50


Example for DT10 with a random dataset:

    bin/dt10.exe random 4 1000 1000 

## Update for apsi.cpp

Example for APSI with a random dataset:

bin/apsi.exe random 4 1000 1000 --mode wan