use crate::fri::*;
use crate::comm_channel::CommunicationChannel;  
use crate::vole_triple::{VoleTriple, PrimalLPNParameterFp61};
use crate::utils::{
    parallel_fft, parallel_ifft, parallel_fft_coset,
    get_roots_of_unity, get_coset, rand_field_element, bit_reverse_permute, xor,
    TWO_ADIC_PRIMITVE_ROOT_OF_UNITY};
use psiri_aes::prg::PRG;
use psiri_aes::prp::FieldPRP;
use psiri_okvs::types::{Okvs, Pair};
use psiri_okvs::okvs::{RbOkvs, BinRbOkvs};
use rand::Rng;
use blake3;
use std::convert::TryInto;
use std::time::Instant;
use std::collections::HashMap;
use lambdaworks_math::traits::{ByteConversion, AsBytes};
use lambdaworks_math::field::fields::fft_friendly::stark_252_prime_field::Stark252PrimeField;
use lambdaworks_crypto::merkle_tree::proof::Proof;
use lambdaworks_crypto::fiat_shamir::is_transcript::IsTranscript;
use stark_platinum_prover::config::{Commitment, BatchedMerkleTreeBackend};
use stark_platinum_prover::fri::{FieldElement, Polynomial};
use stark_platinum_prover::transcript::StoneProverTranscript;
use rayon::prelude::*;

pub type F = Stark252PrimeField;
pub type FE = FieldElement<F>;

const NUM_QUERIES: usize = 128;

pub struct PicsReceiver {
    n: usize,
    log_n: usize,
    vole_receiver: VoleTriple,
    okvs: BinRbOkvs,
    okvs_size: usize,
    P: Vec<FE>,
    P_evaluations: Vec<FE>,
    P_merkle_root: [u8; 32],
    P_commit: FriLayer,
    P_last_value: FE,
    P_fri_layers: Vec<FriLayer>,
    c_evaluations: Vec<FE>,
    XR_merkle_root: [u8; 32],
    XR_hash: Vec<[u8; 32]>,
    roots_of_unity: Vec<FE>,
    roots_of_unity_inv: Vec<FE>,
    small_roots_of_unity_inv: Vec<FE>,
    coset_powers: Vec<FE>,
    coset_inv: Vec<FE>,
    sender_commit: bool,
    receiver_commit: bool,
    receiver_transcript: StoneProverTranscript,
    zetas: Vec<FE>,
}

impl PicsReceiver {
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
        // Setup OKVS seed
        let mut prg = PRG::new(None, 0);
        let mut r = [[0u8; 16]; 3];
        prg.random_block(&mut r);
        let r1 = r[0];
        let r2 = r[1];
        let r3 = r[2];
        let okvs_size: usize = n / 2 * 3;
        let okvs = BinRbOkvs::new(okvs_size, 130, num_okvs_bins, &r1, &r2, &r3);
        io.send_block::<16>(&r);

        let mut vole_triple = VoleTriple::new(1, true, io, param, comm);
        vole_triple.setup_receiver(io, comm);
        vole_triple.extend_initialization();

        let mut public_input_data: Vec<u8> = "3618502788666131213697322783095070105623107215331596699973092056135872020481".to_string().into_bytes();
        public_input_data.extend_from_slice(&n.to_string().into_bytes());
        let sender_transcript = StoneProverTranscript::new(&public_input_data);
        let receiver_transcript = StoneProverTranscript::new(&public_input_data);

        let coset = FE::one();
        
        PicsReceiver {
            n: n,
            log_n: n.ilog2() as usize,
            vole_receiver: vole_triple,
            okvs: okvs,
            okvs_size: okvs_size,
            P: vec![],
            P_evaluations: vec![],
            P_merkle_root: [0u8; 32],
            P_commit: FriLayer::new(
                vec![FE::zero(); 1].as_slice(), 
                MerkleTree::build(vec![[FE::zero(); 2]; 1].as_slice())),
            P_last_value: FE::zero(),
            P_fri_layers: vec![],
            c_evaluations: vec![],
            XR_merkle_root: [0u8; 32],
            XR_hash: vec![],
            roots_of_unity: vec![],
            roots_of_unity_inv: vec![],
            small_roots_of_unity_inv: vec![],
            coset_powers: vec![],
            coset_inv: vec![],
            sender_commit: sender_commit,
            receiver_commit: receiver_commit,
            receiver_transcript: receiver_transcript,
            zetas: vec![],
        }
    }

    pub fn commit_phase<IO: CommunicationChannel>(&mut self, io: &mut IO, values: &[FE], comm: &mut u64) {
        // Just for fun
        let sz = self.n * 4;
        let aa = vec![rand_field_element(); sz];
        let bb = vec![rand_field_element(); sz];
        let start = Instant::now();
        let cc = aa.par_iter()
            .zip(bb.par_iter())
            .map(|(a, b)| a * b)
            .collect::<Vec<FE>>();
        println!("Multiplication of {} elements took {:?}", sz, start.elapsed());

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

        assert_eq!(values.len(), self.n, "Number of values received does not match the expected number of values");

        let input_seed = [1u8; 32];
        let input_hasher = FieldPRP::new(Some(&input_seed));
        let mut hashes = vec![FE::zero(); self.n];
        hashes.copy_from_slice(values);
        input_hasher.permute_block(&mut hashes, self.n);

        let input_kv = values.iter().zip(hashes.iter())
            .map(|(input, hash)| (*input, *hash))
            .collect::<Vec<Pair<FE, FE>>>();
        self.P = self.okvs.encode(&input_kv).expect("Failed to encode using OKVS");
        while self.P.len() < self.n * 2 {
            self.P.push(rand_field_element());
        }

        if self.receiver_commit {
            // Interpolate P
            let P_poly_coeffs = parallel_ifft(&self.P, &self.small_roots_of_unity_inv, self.log_n + 1, 0);
            self.P_evaluations = parallel_fft_coset(&P_poly_coeffs, &self.roots_of_unity, &self.coset_powers, self.log_n + 1, 1);
            self.P_evaluations = bit_reverse_permute(&mut self.P_evaluations, self.n * 4); // makes life easier in folding
            let to_commit: Vec<[FE; 2]> = self.P_evaluations.par_chunks_exact(2).map(|chunk| [chunk[0].clone(), chunk[1].clone()]).collect();
            let P_merkle_tree = MerkleTree::build(&to_commit);
            self.P_merkle_root = P_merkle_tree.root;
        }

        if self.sender_commit {
            self.XR_merkle_root = io.receive_block::<32>().expect("Failed to receive sender merkle root")[0];
            println!("Received merkle");
        }

        if self.receiver_commit {
            // Now can send the commitment of P_new
            *comm += io.send_block::<32>(&[self.P_merkle_root]).expect("Failed to send merkle root of P_new");
            println!("Sent P merkle root");
            self.receiver_transcript.append_bytes(&self.P_merkle_root);
        }
    }

    pub fn receive<IO: CommunicationChannel>(&mut self, io: &mut IO, values: &[FE], comm: &mut u64) {
        // Receive committed ws from Sender
        let mut hws = FE::zero();
        hws = io.receive_stark252().expect("Failed to receive H(ws) from the sender")[0];

        // Running Vole
        // c = b + a * delta
        println!("Communication before Vole: {} bytes", comm);
        let start = Instant::now();
        let mut a = vec![FE::zero(); self.n * 2];
        let mut c = vec![FE::zero(); self.n * 2];    
        self.vole_receiver.extend(io, &mut c, &mut a, self.n * 2, comm);
        println!("Vole receiver took {:?}", start.elapsed());
        println!("Communication of receiver after Vole: {} bytes", comm);

        // Generate random coin if malicious
        let mut wr = FE::zero();
        let mut ws = FE::zero();
        let mut w = FE::zero();
        let mut prg = PRG::new(None, 0);
        let mut wr_vec = [FE::zero(); 1];

        prg.random_stark252_elements(&mut wr_vec);
        wr = wr_vec[0];
        *comm += io.send_stark252(&[wr]).expect("Failed to send wr to the sender");
        ws = io.receive_stark252().expect("Failed to receive ws from the sender")[0];
        let hash_seed = [0u8; 32];
        let hash = FieldPRP::new(Some(&hash_seed));
        let mut ws_vec = [ws];
        hash.permute_block(&mut ws_vec, 1);
        let hws2 = ws_vec[0];
        assert_eq!(hws, hws2, "H(ws) does not match");
        w = ws + wr;

        if self.receiver_commit {
            self.receiver_transcript.append_bytes(&hws.to_bytes_le());
            self.receiver_transcript.append_bytes(&wr.to_bytes_le());
            self.receiver_transcript.append_bytes(&w.to_bytes_le());
        }

        // Compute A = P + a
        let mut A = vec![FE::zero(); self.n * 2];
        for i in 0..(self.n * 2) {
            A[i] = self.P[i] + a[i];
        }
        // Send A = P + a to the sender
        if self.receiver_commit {
            *comm += io.send_stark252(&A).expect("Failed to send A to the sender");
        } else {
            *comm += io.send_stark252(&A[..self.okvs_size]).expect("Failed to send A to the sender");
        }

        let mut o = self.okvs.decode(&c[..self.okvs_size], &values);
        for i in 0..o.len() {
            o[i] = o[i] + w;
        }
        let mut YO: Vec<[FE; 2]> = Vec::with_capacity(self.n);
        for i in 0..self.n {
            YO.push([values[i], o[i]]);
        }

        println!("Until computing all outputs took {:?}", start.elapsed());

        self.prepare_consistency(io, comm, &c);

        println!("Until preparing consistency took {:?}", start.elapsed());

        self.receive_outputs(io, comm, &YO);

        println!("Until receiving outputs took {:?}", start.elapsed());
    }

    pub fn prepare_consistency<IO: CommunicationChannel>(&mut self, io: &mut IO, comm: &mut u64, c: &[FE]) {
        let start = Instant::now();
        if self.sender_commit {
            self.XR_hash = io.receive_block::<32>().expect("Failed to receive XR hash");
            println!("Received hashes: {:?}", &self.XR_hash[..5]);
            // Build Merkle Tree of XR_hash 
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
            assert_eq!(XR_hashes[0], self.XR_merkle_root, "The XR hash does not match the expected merkle root");
            println!("XR hash is consistent with the merkle root");
        }
        println!("Check merkle of XR took {:?}", start.elapsed());

        if self.receiver_commit {
            // Degree check
            (self.P_last_value, self.P_fri_layers, self.zetas) = folding_RS(
                self.log_n + 1,
                self.n * 4,
                &self.P_evaluations,
                &mut self.receiver_transcript,
                &self.coset_inv,
            );

            println!("Folding RS took {:?}", start.elapsed());

            let c_poly_coeffs = parallel_ifft(&c, &self.small_roots_of_unity_inv, self.log_n + 1, 0);
            self.c_evaluations = parallel_fft_coset(&c_poly_coeffs, &self.roots_of_unity, &self.coset_powers, self.log_n + 1, 1);
            self.c_evaluations = bit_reverse_permute(&mut self.c_evaluations, self.n * 4);

            println!("FFT of c took {:?}", start.elapsed());
        }
    }

    pub fn receive_outputs<IO: CommunicationChannel>(&mut self, io: &mut IO, comm: &mut u64, YO: &[[FE; 2]]) {
        let start = Instant::now();
        if self.receiver_commit {
            *comm += io.send_stark252(&[self.P_last_value]).expect("Failed to send last value of P");
            let P_merkle_roots: Vec<[u8; 32]> = self.P_fri_layers.iter()
                .map(|layer| layer.merkle_tree.root)
                .collect();
            *comm += io.send_block::<32>(&P_merkle_roots).expect("Failed to send P merkle roots");
            self.receiver_transcript.append_bytes(&self.P_last_value.to_bytes_le());
            self.receiver_transcript.append_bytes(&P_merkle_roots[0]);

            println!("Until sending P last value and merkle roots took {:?}", start.elapsed());

            // Degree test for P
            let iotas: Vec<usize> = (0..NUM_QUERIES).map(|_| {
                let iota_bytes = self.receiver_transcript.sample(8);
                let iota = u64::from_le_bytes(iota_bytes.clone().try_into().unwrap()) % ((self.n * 4) as u64) as u64;
                self.receiver_transcript.append_bytes(&iota_bytes);
                iota as usize
            }).collect();
            let P_evaluations: Vec<FE> = iotas
                .iter()
                .map(|&iota| self.P_evaluations[iota])
                .collect();
            *comm += io.send_stark252(&P_evaluations).expect("Failed to send evaluations of P");
            let P_decommit = query_RS_folding(&self.P_fri_layers, &iotas, &self.coset_inv, &self.zetas);

            println!("Until querying P took {:?}", start.elapsed());

            send_decommit(io, &P_decommit, comm);

            let c_evaluations: Vec<FE> = iotas
                .iter()
                .map(|&iota| self.c_evaluations[iota])
                .collect();
            *comm += io.send_stark252(&c_evaluations).expect("Failed to send evaluations of c");
        }
        println!("Until sending P and c took {:?}", start.elapsed());

        let mut intersect = vec![false; self.n];
        let XO_hash = io.receive_block::<11>().expect("Failed to receive XO hash");
        let mut sender_kv = HashMap::<[u8; 11], usize>::new();
        XO_hash.iter().enumerate().for_each(|(i, hash)| {
            sender_kv.insert(*hash, i);
        });
        if self.sender_commit {
            let XO_hash2 = io.receive_block::<16>().expect("Failed to receive XO hash 2");
            YO.par_iter().zip(intersect.par_iter_mut()).enumerate().for_each(|(i, (pair, is_intersect))| {
                let mut hasher = blake3::Hasher::new();
                hasher.update(&pair[0].as_bytes());
                hasher.update(&pair[1].as_bytes());
                let mut hash = [0u8; 11];
                hash.copy_from_slice(&hasher.finalize().as_bytes()[..11]);
                let mut r = sender_kv.get(&hash).copied().unwrap_or(self.n + 1);
                if r == self.n + 1 {
                } else {
                    let mut YO_hash2 = XO_hash2[r];
                    let mut hasher2 = blake3::Hasher::new();
                    hasher2.update(&[1u8; 32]);
                    hasher2.update(&pair[0].as_bytes());
                    hasher2.update(&pair[1].as_bytes());
                    let mut hash2 = [0u8; 16];
                    hash2.copy_from_slice(&hasher2.finalize().as_bytes()[..16]);
                    YO_hash2 = xor::<16>(&YO_hash2, &hash2.try_into().unwrap());
                    let mut hasher3 = blake3::Hasher::new();
                    hasher3.update(&pair[0].as_bytes());
                    hasher3.update(&YO_hash2);
                    let mut hash3 = [0u8; 32];
                    hash3.copy_from_slice(hasher3.finalize().as_bytes());
                    if hash3 == self.XR_hash[r] {
                        *is_intersect = true;
                    }
                }
            });
            let mut final_intersect = Vec::<FE>::new();
            for i in 0..self.n {
                if intersect[i] {
                    final_intersect.push(YO[i][0]);
                }
            }
            println!("Receiver received intersection: {:?}", final_intersect.len());
        } else {
            YO.par_iter().zip(intersect.par_iter_mut()).enumerate().for_each(|(i, (pair, is_intersect))| {
                let mut hasher = blake3::Hasher::new();
                hasher.update(&pair[0].as_bytes());
                hasher.update(&pair[1].as_bytes());
                let mut hash = [0u8; 11];
                hash.copy_from_slice(&hasher.finalize().as_bytes()[..11]);
                let mut r = sender_kv.get(&hash).copied().unwrap_or(self.n + 1);
                if r == self.n + 1 {
                    *is_intersect = false;
                } else {
                    *is_intersect = true;
                }
            });
            let mut final_intersect = Vec::<FE>::new();
            for i in 0..self.n {
                if intersect[i] {
                    final_intersect.push(YO[i][0]); 
                }
            }
            println!("Receiver received intersection: {:?}", final_intersect.len());
        }
    }

}