extern crate psiri_okvs;
extern crate lambdaworks_math;

use psiri_okvs::okvs::BinRbOkvs;
use psiri_okvs::utils::rand_field_element;
use lambdaworks_math::field::fields::fft_friendly::stark_252_prime_field::Stark252PrimeField;
use lambdaworks_math::field::element::FieldElement;
use std::time::Instant;

pub type F = Stark252PrimeField;
pub type FE = FieldElement<F>;

fn main() {
    let size = 1 << 20; // 1 million elements
    let columns = 2 * size;
    let band_width = 80;
    let num_bins = 4;

    let mut inputs: Vec<(FE, FE)> = Vec::with_capacity(size);
    for _ in 0..size {
        inputs.push((rand_field_element(), rand_field_element()));
    }

    println!("Inputs: {:?}", &inputs[..5]);

    let r1 = [0u8; 16];
    let r2 = [1u8; 16];
    let r3 = [2u8; 16];

    let okvs = BinRbOkvs::new(columns, band_width, num_bins, &r1, &r2, &r3);

    let start = Instant::now();
    let u = okvs.encode(&inputs).unwrap();
    println!("Time to encode OKVS for {} elements: {:?}", size, start.elapsed());

    for x in &u[..5] {
        println!("{:?}", x);
    }

    let keys: Vec<FE> = inputs.iter().map(|x| x.0.clone()).collect();

    let decoded = okvs.decode(&u, &keys);

    for i in 0..size {
        assert_eq!(inputs[i].1, decoded[i], "Mismatch at index {}", i);
    }
}