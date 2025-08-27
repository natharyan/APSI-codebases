extern crate psiri_vole;
extern crate lambdaworks_math;
extern crate rayon;
extern crate clap;
extern crate rand;
extern crate rand_chacha;

use clap::{Command, Arg};
use psiri_vole::socket_channel::TcpChannel;
use psiri_vole::comm_channel::CommunicationChannel;
use psiri_vole::pics_sender::PicsSender;
use psiri_vole::pics_receiver::PicsReceiver;
use psiri_vole::vole_triple::*;
use std::net::{TcpStream, TcpListener};
use std::time::Instant;
use lambdaworks_math::field::fields::fft_friendly::stark_252_prime_field::Stark252PrimeField;
use lambdaworks_math::field::element::FieldElement;
use lambdaworks_math::unsigned_integer::element::UnsignedInteger;
use rayon::ThreadPoolBuilder;
use rayon::current_num_threads;
use rand::prelude::*;
use rand_chacha::rand_core::{SeedableRng, RngCore};
use rand_chacha::ChaCha12Rng;

pub type F = Stark252PrimeField;
pub type FE = FieldElement<F>;

pub fn gen_input(rng: &mut ChaCha12Rng) -> FE {
    let rand_big = UnsignedInteger {limbs: [rng.next_u64(), rng.next_u64(), rng.next_u64(), rng.next_u64()]} ;
    FE::new(rand_big)
}

fn main() {
    // Command-line argument parsing
    let matches = Command::new("PSI Protocol")
        .version("1.0")
        .author("Your Name")
        .about("Sender and receiver for PSI protocol")
        .arg(
            Arg::new("role")
                .help("Role of this instance (sender or receiver)")
                .required(true)
                .index(1),
        )
        .arg(
            Arg::new("address")
                .help("Address of the other party (e.g., 127.0.0.1)")
                .required(true)
                .index(2),
        )
        .arg(
            Arg::new("port")
                .help("Port to connect on (e.g., 8080)")
                .required(true)
                .index(3),
        )
        .arg(
            Arg::new("log_size")
                .help("Log intersection size")
                .required(true)
                .index(4),
        )
        .arg(
            Arg::new("threads")
                .help("Set the number of threads")
                .required(true)
                .index(5)
        )
        .arg(
            Arg::new("params")
                .help("Choose LPN Param")
                .required(true)
                .index(6)
        )
        .arg(
            Arg::new("sender_commit")
                .help("Either committed or non committed")
                .required(true)
                .index(7)
        )
        .arg(
            Arg::new("receiver_commit")
                .help("Either committed or non committed")
                .required(true)
                .index(8)
        )
        .get_matches();

    let role = matches.get_one::<String>("role").unwrap();
    let address = matches.get_one::<String>("address").unwrap();
    let port = matches.get_one::<String>("port").unwrap();
    let log_size = matches.get_one::<String>("log_size").unwrap().parse::<usize>().unwrap();
    let num_threads = matches.get_one::<String>("threads").unwrap().parse::<usize>().unwrap();
    let params_idx = matches.get_one::<String>("params").unwrap().parse::<usize>().unwrap();
    let sender_commit = matches.get_one::<String>("sender_commit").unwrap().parse::<usize>().unwrap() != 0;
    let receiver_commit = matches.get_one::<String>("receiver_commit").unwrap().parse::<usize>().unwrap() != 0;

    let available_num_threads = vec![1, 2, 4, 8, 16, 32, 64];
    if !available_num_threads.contains(&num_threads) {
        panic!("Currently we only support threads number to be power of 2.");
    }
    
    ThreadPoolBuilder::new()
        .num_threads(num_threads) // Adjust the number of threads as needed
        .build_global()
        .unwrap();

    println!("🚀 Rayon is using {} threads", num_threads);
    println!("Sender commit? {}", sender_commit);
    println!("Receiver commit? {}", receiver_commit);

    let size = 1 << log_size;
    let mut param = LPN17;
    if params_idx == 0 {
        param = LPN17;
    } else if params_idx == 1{
        param = LPN21;
    } else if params_idx == 2 {
        param = LPN25;
    }

    let mut comm: u64 = 0;

    if role == "sender" {
        // Sender logic
        println!("Starting as Sender...");
        let stream = TcpStream::connect(format!("{}:{}", address, port))
            .expect("Failed to connect to receiver");
        let mut channel = TcpChannel::new(stream);

        let seed = channel.receive_block::<16>().expect("Failed to receive seed from receiver");
        let mut rng = ChaCha12Rng::from_seed(Default::default());
        let data = (0..size).map(|_| gen_input(&mut rng)).collect::<Vec<FE>>();

        println!("Started PSI");

        let mut oprf = PicsSender::new(&mut channel, size, size * 3 / 2, num_threads, sender_commit, receiver_commit, param, &mut comm);
        println!("OPRF initialized");

        let start = Instant::now();
        let current_comm = comm.clone();
        oprf.commit_phase(&mut channel, &data, &mut comm);
        println!("Sender commit phase time: {:?}", start.elapsed());
        let sender_comm = comm - current_comm;
        let sender_comm_mb = sender_comm as f64 / (1024.0 * 1024.0);
        println!("Sender committed data size: {} MB", sender_comm_mb);

        oprf.send(&mut channel, &mut comm);

        channel.send_block::<8>(&[comm.to_le_bytes()]).expect("Failed to send total communication size to receiver");

        let receiver_signal = channel.receive_block::<8>().expect("Failed to receive complete signal from receiver");   
        println!("Sender received complete signal from receiver: {:?}", receiver_signal);
    } else if role == "receiver" {
        // Receiver logic
        println!("Starting as Receiver...");
        let listener = TcpListener::bind(format!("{}:{}", address, port))
            .expect("Failed to bind to port");
        let (stream, _) = listener.accept().expect("Failed to accept connection");
        let mut channel = TcpChannel::new(stream);

        // Send data to Sender for test
        let mut seed = [0u8; 16];
        let mut rng_seed = rand::thread_rng();
        rng_seed.fill(&mut seed);
        let mut rng = ChaCha12Rng::from_seed(Default::default());
        let data = (0..size).map(|_| gen_input(&mut rng)).collect::<Vec<FE>>();

        channel.send_block::<16>(&[seed]).expect("Failed to send seed to sender");
        println!("Started PSI");

        let start_protocol = Instant::now();

        let mut oprf = PicsReceiver::new(&mut channel, size, size * 3 / 2, num_threads, sender_commit, receiver_commit, param, &mut comm);

        let start = Instant::now();
        let current_comm = comm.clone();
        oprf.commit_phase(&mut channel, &data, &mut comm);
        println!("Receiver commit phase time: {:?}", start.elapsed());
        let committed_data_size = comm - current_comm;
        let committed_data_size_mb = committed_data_size as f64 / (1024.0 * 1024.0);
        println!("Receiver committed data size: {} MB", committed_data_size_mb);

        let mut outputs = vec![FE::zero(); size];
        oprf.receive(&mut channel, &data, &mut comm);
        println!("Whole protocol time: {:?}", start_protocol.elapsed());

        let sender_comm_bytes = channel.receive_block::<8>().expect("Failed to receive total communication size from sender")[0];
        let sender_comm = u64::from_le_bytes(sender_comm_bytes);
        let total_comm = comm + sender_comm;
        let total_comm_mb = total_comm as f64 / (1024.0 * 1024.0);
        println!("Total communication size sender and receiver: {} MB", total_comm_mb);

        channel.send_block::<8>(&[(1u64).to_le_bytes()]).expect("Failed to send complete signal to sender");
    } else {
        eprintln!("Invalid role. Use 'sender' or 'receiver'.");
        std::process::exit(1);
    }

    println!("Total data sent: {} bytes", comm);
}