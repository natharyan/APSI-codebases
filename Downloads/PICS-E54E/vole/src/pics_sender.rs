use crate::fri::*;
use crate::utils::{
    rand_field_element, xor, bit_reverse_permute,
    get_roots_of_unity, get_coset, TWO_ADIC_PRIMITVE_ROOT_OF_UNITY,
    parallel_fft, parallel_ifft, parallel_fft_coset};
use crate::comm_channel::CommunicationChannel;
use crate::vole_triple::{VoleTriple, PrimalLPNParameterFp61};
use psiri_aes::prg::PRG;
use psiri_aes::prp::FieldPRP;
use psiri_okvs::types::Okvs;
use psiri_okvs::okvs::{RbOkvs, BinRbOkvs};
use rand::Rng;
use blake3;
use std::time::Instant;
use std::convert::TryInto;
use std::cmp::min;
use lambdaworks_math::traits::{ByteConversion, AsBytes};
use lambdaworks_math::fft::cpu::bit_reversing::{reverse_index, in_place_bit_reverse_permute};
use lambdaworks_math::field::fields::fft_friendly::stark_252_prime_field::Stark252PrimeField;
use lambdaworks_crypto::merkle_tree::proof::Proof;
use lambdaworks_crypto::fiat_shamir::is_transcript::IsTranscript;
use stark_platinum_prover::transcript::StoneProverTranscript;
use stark_platinum_prover::fri::{FieldElement, Polynomial};
use stark_platinum_prover::config::Commitment;
use stark_platinum_prover::fri::fri_decommit::FriDecommitment;
use rayon::prelude::*;  
use rayon::current_num_threads;

pub type F = Stark252PrimeField;
pub type FE = FieldElement<F>;

const NUM_QUERIES: usize = 128;

pub struct PicsSender {
    n: usize,
    log_n: usize,
    vole_sender: VoleTriple,
    delta: FE,
    okvs: BinRbOkvs,
    okvs_size: usize,
    w: FE, 
    P_merkle_root: [u8; 32],
    outputs_byte: Vec<[u8; 32]>,
    XR: Vec<(FE, [u8; 16])>,
    XR_hash: Vec<[u8; 32]>,
    XO_hash: Vec<[u8; 11]>,
    XO_hash2: Vec<[u8; 16]>,
    b_evaluations: Vec<FE>,
    A_evaluations: Vec<FE>,
    coset_powers: Vec<FE>,
    coset_inv: Vec<FE>, 
    roots_of_unity: Vec<FE>,
    roots_of_unity_inv: Vec<FE>,
    small_roots_of_unity_inv: Vec<FE>,
    sender_commit: bool,
    receiver_commit: bool,
    sender_transcript: StoneProverTranscript,
    receiver_transcript: StoneProverTranscript,
}

impl PicsSender {
    pub fn new<IO: CommunicationChannel>(
        io: &mut IO, 
        n: usize, 
        okvs_size: usize,
        num_okvs_bins: usize, 
        sender_commit: bool,
        receiver_commit: bool,
        param: PrimalLPNParameterFp61, 
        comm: &mut u64
    ) -> Self {
        // Setup delta
        let mut prg = PRG::new(None, 0);
        let mut FE_vec = [FE::zero(); 1];
        prg.random_stark252_elements(&mut FE_vec);
        let delta = FE_vec[0]; 

        // Receive OKVS seed from the receiver
        let r = io.receive_block::<16>().expect("Failed to receive okvs seed");
        let r1 = r[0];
        let r2 = r[1];
        let r3 = r[2];
        let okvs = BinRbOkvs::new(okvs_size, 130, num_okvs_bins, &r1, &r2, &r3);

        let mut vole_triple = VoleTriple::new(0, true, io, param, comm);
        vole_triple.setup_sender(io, delta, comm);
        vole_triple.extend_initialization();

        let mut public_input_data: Vec<u8> = "3618502788666131213697322783095070105623107215331596699973092056135872020481".to_string().into_bytes();
        public_input_data.extend_from_slice(&n.to_string().into_bytes());
        let sender_transcript = StoneProverTranscript::new(&public_input_data);
        let receiver_transcript = StoneProverTranscript::new(&public_input_data);

        PicsSender {
            n: n,
            log_n: n.ilog2() as usize,
            vole_sender: vole_triple,
            delta: delta,
            okvs: okvs,
            okvs_size: okvs_size,
            w: FE::zero(),
            P_merkle_root: [0u8; 32],
            outputs_byte: vec![],
            XR: vec![],
            XR_hash: vec![],
            XO_hash: vec![],
            XO_hash2: vec![],
            b_evaluations: vec![],
            A_evaluations: vec![],
            coset_powers: vec![],
            coset_inv: vec![],
            roots_of_unity: vec![],
            roots_of_unity_inv: vec![],
            small_roots_of_unity_inv: vec![],
            sender_commit: sender_commit,
            receiver_commit: receiver_commit,
            sender_transcript: sender_transcript,
            receiver_transcript: receiver_transcript,
        }
    }

    pub fn commit_phase<IO: CommunicationChannel>(&mut self, io: &mut IO, values: &[FE], comm: &mut u64) {
        assert_eq!(values.len(), self.n, "The number of values of Sender should be n");
        println!("Starting commit phase of Sender");

        // Prepare roots of unity if there is commitment
        if self.receiver_commit {
            let start = Instant::now();
            self.roots_of_unity = get_roots_of_unity((self.log_n + 2) as u64);
            println!("Roots of unity prepared in {:?}", start.elapsed());
            self.roots_of_unity_inv = self.roots_of_unity.clone();
            self.roots_of_unity_inv[1..].reverse();
            self.small_roots_of_unity_inv = (0..(self.n * 2)).into_par_iter().map(|i| self.roots_of_unity_inv[i*2]).collect();
            self.coset_powers = get_coset(FE::new(TWO_ADIC_PRIMITVE_ROOT_OF_UNITY), (self.n * 4) as u64);
            let g_inv = self.coset_powers[1].inv().expect("Coset element is zero");
            self.coset_inv = self.roots_of_unity_inv
                .par_iter()
                .enumerate()
                .map(|(i, x)| x * g_inv)
                .collect::<Vec<FE>>();
            self.coset_inv = bit_reverse_permute(&mut self.coset_inv, self.n * 4);
            println!("Roots of unity and coset prepared in {:?}", start.elapsed());
        }

        self.XR = Vec::<(FE, [u8; 16])>::with_capacity(self.n);
        for i in 0..self.n {
            self.XR.push((values[i], rand_field_element().to_bytes_le()[..16].try_into().unwrap()));
        }
        if self.sender_commit {
            self.XR_hash = self.XR.par_iter().map(|x| {
                let mut to_be_hashed = Vec::<u8>::new();
                to_be_hashed.extend_from_slice(&x.0.as_bytes());
                to_be_hashed.extend_from_slice(&x.1);
                let hash = blake3::hash(&to_be_hashed);
                let mut hashed = [0u8; 32];
                hashed.copy_from_slice(hash.as_bytes());
                hashed
            }).collect();

            let mut XR_hashes: Vec<[u8; 32]> = self.XR_hash.clone();
            let mut size = self.XR_hash.len();
            while size > 1 {
                let mut new_XR_hashes = vec![[0u8; 32]; size / 2];
                new_XR_hashes.par_iter_mut().enumerate().for_each(|(i, hash)| {
                    let mut to_be_hashed = Vec::<u8>::new();
                    to_be_hashed.extend_from_slice(&XR_hashes[2 * i]);
                    to_be_hashed.extend_from_slice(&XR_hashes[2 * i + 1]);
                    let hash0 = blake3::hash(&to_be_hashed);
                    hash.copy_from_slice(hash0.as_bytes());
                });
                XR_hashes[..size / 2].copy_from_slice(&new_XR_hashes);
                size >>= 1;
            }

            let sender_merkle_root = XR_hashes[0];
            *comm += io.send_block::<32>(&[XR_hashes[0]])
                .expect("Failed to send merkle root of X");
        }

        if self.receiver_commit {
            self.P_merkle_root = io.receive_block::<32>().expect("Failed to receive merkle root of P")[0];
            self.receiver_transcript.append_bytes(&self.P_merkle_root);
        }
    }

    pub fn send<IO: CommunicationChannel>(&mut self, io: &mut IO, comm: &mut u64) {
        // Creating ws and send H(ws) to the receiver
        let mut prg = PRG::new(None, 0);
        let mut FE_vec = [FE::zero(); 1];
        prg.random_stark252_elements(&mut FE_vec);
        let ws = FE_vec[0];
        let mut hash_buf = FE::zero();

        let hash_seed = [0u8; 32];
        let hash = FieldPRP::new(Some(&hash_seed));
        let mut ws_vec = [ws];
        hash.permute_block(&mut ws_vec, 1);
        hash_buf = ws_vec[0];
        *comm += io.send_stark252(&[hash_buf]).expect("Failed to send H(ws) to the receiver");

        println!("Communication before vole: {} bytes", comm);
        // Running Vole
        // c = b + a * delta
        let mut z = vec![FE::zero(); self.n * 2];
        let mut b = vec![FE::zero(); self.n * 2];
        self.vole_sender.extend(io, &mut b, &mut z, self.n * 2, comm); 

        println!("Communication after vole: {} bytes", comm);

        // Get w = ws + wr;
        let wr = io.receive_stark252().expect("Failed to receive wr from the receiver")[0];
        self.w = ws + wr;
        *comm += io.send_stark252(&[ws]).expect("Failed to send ws to the receiver");

        if self.receiver_commit {
            self.receiver_transcript.append_bytes(&hash_buf.to_bytes_le());
            self.receiver_transcript.append_bytes(&wr.to_bytes_le());
            self.receiver_transcript.append_bytes(&self.w.to_bytes_le());
        }

        let A = io.receive_stark252().expect("Failed to receive A from the receiver");

        let mut K = vec![FE::zero(); self.okvs_size];
        K.par_iter_mut().enumerate().for_each(|(i, Ki)| {
            *Ki = b[i] + A[i] * self.delta;
        });

        let values = self.XR.par_iter().map(|x| x.0).collect::<Vec<FE>>();
        let mut o = self.okvs.decode(&K[..self.okvs_size], &values);
        let input_seed = [1u8; 32];
        let input_hasher = FieldPRP::new(Some(&input_seed));
        let mut hashes = self.XR.par_iter().map(|x| x.0).collect::<Vec<FE>>();
        input_hasher.permute_block(&mut hashes, self.n);
        o.par_iter_mut().enumerate().for_each(|(i, oi)| {
            *oi = *oi - self.delta * hashes[i] + self.w;
        });
        let mut XO = Vec::<[FE; 2]>::with_capacity(self.n);
        for i in 0..self.n {
            XO.push([values[i], o[i]]);
        }

        self.prepare_consistency(io, comm, &XO, &A, &b);
        self.send_outputs(io, comm);
    }

    fn prepare_consistency<IO: CommunicationChannel>(&mut self, io: &mut IO, comm: &mut u64, XO: &[[FE; 2]], A: &[FE], b: &[FE]) {
        self.XO_hash = XO.par_iter().map(|x| {
            let mut hasher = blake3::Hasher::new();
            hasher.update(&x[0].as_bytes());
            hasher.update(&x[1].as_bytes());
            let mut hash = [0u8; 11];
            hash.copy_from_slice(&hasher.finalize().as_bytes()[..11]);
            hash
        }).collect();

        if self.sender_commit {
            *comm += io.send_block::<32>(&self.XR_hash)
                .expect("Failed to send XR hashes");
            self.XO_hash2 = XO.par_iter().enumerate().map(|(i, x)| {
                let mut hasher = blake3::Hasher::new();
                hasher.update(&[1u8; 32]);
                hasher.update(&x[0].as_bytes());
                hasher.update(&x[1].as_bytes());
                let mut hash = [0u8; 16];
                hash.copy_from_slice(&hasher.finalize().as_bytes()[..16]);
                xor::<16>(&hash, &self.XR[i].1)
            }).collect(); // Hashes of XO_i, xor with R_i
        }

        if self.receiver_commit {
            let b_poly_coeffs = parallel_ifft(&b, &self.small_roots_of_unity_inv, self.log_n + 1, 0);
            let A_poly_coeffs = parallel_ifft(&A, &self.small_roots_of_unity_inv, self.log_n + 1, 0);
            self.b_evaluations = parallel_fft_coset(&b_poly_coeffs, &self.roots_of_unity, &self.coset_powers, self.log_n + 1, 1);
            self.A_evaluations = parallel_fft_coset(&A_poly_coeffs, &self.roots_of_unity, &self.coset_powers, self.log_n + 1, 1);
            self.b_evaluations = bit_reverse_permute(&mut self.b_evaluations, self.n * 4);
            self.A_evaluations = bit_reverse_permute(&mut self.A_evaluations, self.n * 4);
        }
    }

    fn send_outputs<IO: CommunicationChannel>(&mut self, io: &mut IO, comm: &mut u64) {
        if self.receiver_commit {
            let P_last_value = io.receive_stark252().expect("Failed to receive last value of P")[0];
            let P_merkle_roots: Vec<[u8; 32]> = io.receive_block::<32>().expect("Failed to receive P merkle roots");
            let mut zetas = P_merkle_roots
                .iter()
                .map(|root| {
                    // >>>> Send challenge 𝜁ₖ
                    self.receiver_transcript.append_bytes(root);
                    let element = self.receiver_transcript.sample_field_element();
                    // <<<< Receive commitment: [pₖ] (the first one is [p₀])
                    element
                })
                .collect::<Vec<FE>>();

            zetas.push(self.receiver_transcript.sample_field_element());

            self.receiver_transcript.append_bytes(&P_last_value.to_bytes_le());
            self.receiver_transcript.append_bytes(&P_merkle_roots[0]);

            let iotas = (0..NUM_QUERIES).map(|_| {
                let iota_bytes = self.receiver_transcript.sample(8);
                let iota = u64::from_le_bytes(iota_bytes.clone().try_into().unwrap()) % ((self.n * 4) as u64) as u64;
                self.receiver_transcript.append_bytes(&iota_bytes);
                iota as usize
            }).collect::<Vec<usize>>();
            let P_evaluations = io.receive_stark252().expect("Failed to receive evaluations of P");
            let P_decommit = receive_decommit(io, comm);

            iotas.par_iter().enumerate().for_each(|(i, &iota)| {
                let result = verify_RS_folding(
                    P_last_value,
                    &P_merkle_roots,
                    &P_decommit[i],
                    iota,
                    P_evaluations[i],
                    P_decommit[i].layers_evaluations_sym[0],
                    &self.coset_inv,
                    &zetas,
                );
                if !result {
                    panic!("Receiver lied about P at iota = {}", iota);
                }
            });
            println!("P passed the degree test!");

            let c_evaluations = io.receive_stark252().expect("Failed to receive evaluations of c");
            let b_evaluations = iotas
                .par_iter()
                .map(|&iota| self.b_evaluations[iota])
                .collect::<Vec<FE>>();
            let A_evaluations = iotas
                .par_iter()
                .map(|&iota| self.A_evaluations[iota])
                .collect::<Vec<FE>>();

            let delta_inv = self.delta.inv().expect("Delta is zero");
            for i in 0..NUM_QUERIES {
                if (c_evaluations[i] - b_evaluations[i]) * delta_inv != A_evaluations[i] - P_evaluations[i] {
                    panic!("VOLE consistency check failed at iota = {}", iotas[i]);
                }
            }
        }

        *comm += io.send_block::<11>(&self.XO_hash)
            .expect("Failed to send XO hashes");
        if self.sender_commit {
            *comm += io.send_block::<16>(&self.XO_hash2)
                .expect("Failed to send XO hashes2");
        } else {
        }
    }
}
