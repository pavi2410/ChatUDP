use std::env::args;
use std::io::{stdin, stdout, Write};
use std::net::UdpSocket;
use std::process::exit;
use std::thread;

fn main() {
    let user_name = args().nth(1).expect("Usage: client <user_name>");

    let socket = UdpSocket::bind("127.0.0.1:0").unwrap();
    socket.connect("127.0.0.1:2410").unwrap();

    socket.send(user_name.as_bytes()).unwrap();

    let socket_clone = socket.try_clone().unwrap();

    let sender = thread::spawn(move || {
        loop {
            print!("[{}] ", user_name);
            stdout().flush().unwrap();
            let mut msg = String::new();
            stdin().read_line(&mut msg).unwrap();
            socket.send(msg.as_bytes()).unwrap();

            if msg.trim() == "/quit" {
                exit(0);
            }
        }
    });

    let reader = thread::spawn(move || {
        loop {
            let mut buf = [0; 1024];
            socket_clone.recv_from(&mut buf).unwrap_or_else(|e| {
                println!("Error receiving data: {}", e);
                println!("Server is down. Exiting...");
                exit(0);
            });
            println!("\r{}", String::from_utf8_lossy(&buf));
        }
    });

    sender.join().unwrap();
    reader.join().unwrap();
}
