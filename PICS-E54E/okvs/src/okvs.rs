use crate::types::{Okvs, Pair};
use crate::error::{Result, Error};
use crate::utils::*;

use sp_core::U256;
use lambdaworks_math::field::fields::fft_friendly::stark_252_prime_field::Stark252PrimeField;
use lambdaworks_math::field::element::FieldElement;
use std::time::Instant;
use rayon::prelude::*;

pub type F = Stark252PrimeField;
pub type FE = FieldElement<F>;

const EPSILON: f64 = 1.0; // can change
const BAND_WIDTH: usize = 80; // can change

pub struct RbOkvs {
    pub columns: usize, 
    band_width: usize,
    r1: [u8; 16],
    r2: [u8; 16],
}

impl RbOkvs {
    pub fn new(kv_count: usize, r1: &[u8; 16], r2: &[u8; 16]) -> Self {
        let columns = ((1.0 + EPSILON) * kv_count as f64) as usize;

        Self {
            columns,
            band_width: if BAND_WIDTH < columns {
                BAND_WIDTH
            } else {
                columns * 80 / 100
            },
            r1: *r1,
            r2: *r2,
        }
    }
}

impl Okvs for RbOkvs {
    fn encode(&self, input: &Vec<Pair<FE, FE>>) -> Result<Vec<FE>> {
        let start = Instant::now();
        let (matrix, start_pos, y) = create_sorted_matrix(self.columns, self.band_width, &self.r1, &self.r2, input)?;
        simple_gauss(y, matrix, start_pos, self.columns, self.band_width)
    }

    fn decode(&self, encoding: &Vec<FE>, key: &[FE]) -> Vec<FE> {
        let n = key.len();
        let mut start = vec![0usize; n];
        let mut band = vec![U256::default(); n];

        start.par_iter_mut().zip(band.par_iter_mut()).enumerate().for_each(|(i, (starti, bandi))| {
            *starti = hash_to_index(key[i], &self.r1, self.columns - self.band_width);
            *bandi = hash_to_band(key[i], &self.r2);
        });

        let mut res = vec![FE::zero(); n];

        res.par_iter_mut().enumerate().for_each(|(i, resi)| {
            *resi = inner_product(&band[i], &encoding[start[i]..start[i]+self.band_width]);
        });

        res
    }
}

pub struct BinRbOkvs {
    pub columns_per_bin: usize, 
    band_width: usize,
    num_bins: usize,
    r1: [u8; 16],
    r2: [u8; 16],
    r3: [u8; 16],
}

impl BinRbOkvs {
    pub fn new(columns: usize, band_width: usize, num_bins: usize, r1: &[u8; 16], r2: &[u8; 16], r3: &[u8; 16]) -> Self {
        assert_eq!(columns % num_bins, 0, "Currently only supports number of columns divisible by number of bins");
        Self {
            columns_per_bin: columns / num_bins,
            band_width,
            num_bins,
            r1: *r1,
            r2: *r2,
            r3: *r3,
        }
    }

    pub fn encode(&self, input: &Vec<Pair<FE, FE>>) -> Result<Vec<FE>> {
        let n = input.len();
        let mut bin = vec![self.num_bins; n];
        bin.par_iter_mut().enumerate().for_each(|(i, bin_i)| {
            *bin_i = hash_to_bin(input[i].0, &self.r3, self.num_bins);
        });

        let mut bin_kvs = vec![Vec::new(); self.num_bins];
        for i in 0..n {
            bin_kvs[bin[i]].push(input[i].clone());
        }

        let mut bin_res = vec![Vec::new(); self.num_bins];
        let mut err = vec![false; self.num_bins];

        bin_kvs.par_iter_mut().zip(bin_res.par_iter_mut()).zip(err.par_iter_mut()).for_each(|((bin_kv, bin_rs), bin_err)| {
            let (matrix, start_pos, y) = create_sorted_matrix(self.columns_per_bin, self.band_width, &self.r1, &self.r2, bin_kv).expect("Failed to create sorted matrix");
            let res = simple_gauss(y, matrix, start_pos, self.columns_per_bin, self.band_width);
            if let Ok(rs) = res {
                *bin_rs = rs;
            } else {
                *bin_err = true;
            }
        });

        for i in 0..self.num_bins {
            if err[i] {
                return Err(Error::ZeroRow(i));
            }
        }

        let mut res = Vec::new();
        for i in 0..self.num_bins {
            res.extend(bin_res[i].iter());
        }

        Ok(res)
    }

    pub fn decode(&self, encoding: &[FE], key: &[FE]) -> Vec<FE> {
        let n = key.len();
        let mut start = vec![0usize; n];
        let mut band = vec![U256::default(); n];
        let mut bin = vec![self.num_bins; n];

        start.par_iter_mut().zip(band.par_iter_mut()).zip(bin.par_iter_mut()).enumerate().for_each(|(i, ((starti, bandi), bini))| {
            *starti = hash_to_index(key[i], &self.r1, self.columns_per_bin - self.band_width);
            *bandi = hash_to_band(key[i], &self.r2);
            *bini = hash_to_bin(key[i], &self.r3, self.num_bins);
        });

        let mut res = vec![FE::zero(); n];

        res.par_iter_mut().enumerate().for_each(|(i, resi)| {
            let start_i = start[i] + bin[i] * self.columns_per_bin;
            let end_i = start_i + self.band_width;
            *resi = inner_product(&band[i], &encoding[start_i..end_i]);
        });

        res
    }
}

fn create_sorted_matrix(
    columns: usize,
    band_width: usize,
    r1: &[u8; 16],
    r2: &[u8; 16],
    input: &Vec<Pair<FE, FE>>,
) -> Result<(Vec<U256>, Vec<usize>, Vec<FE>)> {
    let n = input.len();
    let mut start_pos: Vec<(usize, usize)> = vec![(0, 0); n];
    let mut matrix: Vec<U256> = vec![U256::default(); n];
    let mut start_ids: Vec<usize> = vec![0; n];
    let mut y = vec![FE::zero(); n];

    start_pos.par_iter_mut().enumerate().for_each(|(i, start_pos_i)| {
        *start_pos_i = (i, hash_to_index(input[i].0, r1, columns - band_width));
    });

    radix_sort(&mut start_pos, columns - band_width - 1);

    matrix.par_iter_mut().enumerate().for_each(|(i, matrix_i)| {
        *matrix_i = hash_to_band(input[start_pos[i].0].0, r2);
    });

    y.par_iter_mut().enumerate().for_each(|(i, y_i)| {
        *y_i = input[start_pos[i].0].1.to_owned();
    });

    for i in 0..n {
        start_ids[i] = start_pos[i].1;
    }

    Ok((matrix, start_ids, y))
}