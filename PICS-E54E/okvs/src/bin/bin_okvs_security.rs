extern crate psiri_okvs;
extern crate lambdaworks_math;
extern crate rand;
extern crate rayon;

use psiri_okvs::okvs::BinRbOkvs;
use psiri_okvs::types::{Pair, Okvs};
use psiri_okvs::utils::rand_field_element;
use lambdaworks_math::field::fields::fft_friendly::stark_252_prime_field::Stark252PrimeField;
use lambdaworks_math::field::element::FieldElement;
use std::time::Instant;
use rand::Rng;
use rayon::prelude::*;
use std::env;

pub type F = Stark252PrimeField;
pub type FE = FieldElement<F>;

fn main() {
    let arg = env::args().nth(1).expect("Please provide a number");
    let band_width: usize = arg.parse().unwrap();
    println!("Band width: {}", band_width);
    let num_trials = 1 << 22;
    let size = 1 << 12;    
    let columns = 2 * size;
    let band_width = 25;
    let mut inputs: Vec<Pair<FE, FE>> = Vec::with_capacity(size);
    for i in 0..size {
        inputs.push((rand_field_element(), rand_field_element()));
    }

    let num_threads = 128;
    println!("Number of threads: {}", num_threads);
    let mut fails = vec![0usize; num_threads];

    let start = Instant::now();

    fails.par_iter_mut().enumerate().for_each(|(i, fail_count)| {
        let mut rng = rand::thread_rng();
        for _ in 0..num_trials / num_threads {
            let mut r1 = [0u8; 16];
            let mut r2 = [1u8; 16];
            let mut r3 = [2u8; 16];
            
            rng.fill(&mut r1);
            rng.fill(&mut r2);
            rng.fill(&mut r3);

            let okvs = BinRbOkvs::new(columns, band_width, 32, &r1, &r2, &r3);

            if let Err(_) = okvs.encode(&inputs) {
                *fail_count += 1;
            }
        }
    });

    println!("Fails per thread: {:?}", fails);
    let total_fails: usize = fails.iter().sum();
    println!("Total fails: {} over {} trials", total_fails, num_trials);
    println!("Security bits: {}", ((num_trials as f64) / (total_fails as f64)).log2());
    println!("Time taken: {:?}", start.elapsed());
}
