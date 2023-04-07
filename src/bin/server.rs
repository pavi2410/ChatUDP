use std::collections::HashMap;
use std::net::{SocketAddr, UdpSocket};

struct Client {
    user_name: String,
}

pub struct ChatServer {
    server_socket: UdpSocket,
    clients: HashMap<SocketAddr, Client>,
}

impl ChatServer {
    fn new() -> Self {
        Self {
            server_socket: UdpSocket::bind("127.0.0.1:2410").unwrap(),
            clients: HashMap::new(),
        }
    }

    fn is_connected(&self, sender: &SocketAddr) -> bool {
        self.clients.contains_key(&sender)
    }

    fn connect_client(&mut self, sender: &SocketAddr, user_name: String) {
        self.clients.insert(*sender, Client { user_name });
    }

    fn disconnect_client(&mut self, sender: &SocketAddr) {
        self.clients.remove(sender);
    }

    fn get_client(&self, sender: &SocketAddr) -> Option<&Client> {
        self.clients.get(sender)
    }

    fn get_client_list(&self) -> Vec<String> {
        self.clients.values().map(|client| client.user_name.clone()).collect()
    }

    fn broadcast(&self, msg: &str, sender: &SocketAddr) {
        let sender_name = self.get_client(sender).unwrap().user_name.clone();
        for socket in self.clients.keys() {
            if socket == sender {
                continue;
            }
            let msg = format!("[{}] {}", sender_name, msg);
            self.server_socket.send_to(msg.as_bytes(), socket).unwrap();
        }
    }

    fn handle_connection(&mut self, msg: String, sender: &SocketAddr) {
        if self.is_connected(sender) {
            println!("{} is already connected", sender);
            self.handle_msg(msg, sender);
        } else {
            println!("{} is connecting as {}", sender, msg);
            self.connect_client(sender, msg);
            self.broadcast("has joined the chat", sender);
        }
    }

    fn handle_msg(&mut self, msg: String, sender: &SocketAddr) {
        if msg.starts_with("/quit") {
            println!("{} is disconnecting", sender);
            self.broadcast("has left the chat", sender);
            self.disconnect_client(sender);
        } else if msg.starts_with("/list") {
            println!("{} is requesting a list of connected clients", sender);
            let client_list = self.get_client_list();
            self.server_socket.send_to(client_list.join(", ").as_bytes(), sender).unwrap();
        } else {
            self.broadcast(msg.as_str(), sender);
        }
    }

    pub fn init() {
        let mut chat_server = Self::new();
        println!("Server started on 127.0.0.1:2410");
        chat_server.run_loop();
    }

    pub fn run_loop(&mut self) {
        loop {
            let (msg, sender) = read_socket(&self.server_socket);
            self.handle_connection(msg, &sender);
        }
    }
}

fn read_socket(socket: &UdpSocket) -> (String, SocketAddr) {
    let mut buf = [0; 1024];
    let (amt, src) = socket.recv_from(&mut buf).unwrap();
    let msg = String::from_utf8_lossy(&buf[..amt]).to_string();
    (msg, src)
}

fn main() {
    ChatServer::init();
}
