extern crate psiri_okvs;
extern crate lambdaworks_math;
extern crate rand;
extern crate rayon;

use psiri_okvs::okvs::{RbOkvs};
use psiri_okvs::types::{Pair, Okvs};
use psiri_okvs::utils::rand_field_element;
use lambdaworks_math::field::fields::fft_friendly::stark_252_prime_field::Stark252PrimeField;
use lambdaworks_math::field::element::FieldElement;
use std::time::Instant;
use rand::Rng;
use rayon::prelude::*;

pub type F = Stark252PrimeField;
pub type FE = FieldElement<F>;

fn main() {
    let num_trials = 1 << 20;
    let size = 1 << 10;    
    let mut inputs: Vec<Pair<FE, FE>> = Vec::with_capacity(size);
    for i in 0..size {
        inputs.push((rand_field_element(), rand_field_element()));
    }

    let num_threads = rayon::current_num_threads();
    println!("Number of threads: {}", num_threads);
    let mut fails = vec![0usize; num_threads];

    let start = Instant::now();

    fails.par_iter_mut().enumerate().for_each(|(i, fail_count)| {
        let mut rng = rand::thread_rng();
        for _ in 0..num_trials / num_threads {
            let mut r1 = [0u8; 16];
            let mut r2 = [1u8; 16];
            
            rng.fill(&mut r1);
            rng.fill(&mut r2);

            let okvs = RbOkvs::new(size, &r1, &r2);

            if let Err(_) = okvs.encode(&inputs) {
                *fail_count += 1;
            }
        }
    });

    println!("Fails per thread: {:?}", fails);
    let total_fails: usize = fails.iter().sum();
    println!("Total fails: {} over {} trials", total_fails, num_trials);
    println!("Time taken: {:?}", start.elapsed());
}
