use std::{env,};
use std::error::Error;
use std::net::SocketAddr;
use std::sync::{Arc};
use dotenv::dotenv;
use futures::executor::block_on;
use chrono::Utc;

use futures_channel::mpsc::{unbounded, UnboundedSender};
use futures_util::{future, pin_mut, stream::TryStreamExt, StreamExt};

use tokio::net::{TcpListener, TcpStream};
use tokio::sync::Mutex;
use tokio_tungstenite::tungstenite::protocol::Message;
use tungstenite::{handshake::server::{ErrorResponse, Request, Response}, http::Uri};

#[derive(Clone)]
struct Client {
    socket_addr: SocketAddr,
    tx: Tx,
    uri: Uri,
}

type Tx = UnboundedSender<Message>;
type PeerMap = Arc<Mutex<Vec<Client>>>;

async fn handle_connection(peer_map: PeerMap, raw_stream: TcpStream, addr: SocketAddr) {
    println!("Incoming TCP connection from: {}", addr);

    let mut device_uri: Option<Uri> = None;
    let copy_headers_callback = |request: &Request, response: Response| -> Result<Response, ErrorResponse> {
        let uri = Request::uri(request);
        println!("URI: {}", uri);

        device_uri = Some(uri.clone());

        for (name, value) in request.headers().iter() {
            println!("Name: {}, value: {}", name, value.to_str().expect("expected a value"));
        }

        Ok(response)
    };

    let ws_stream = tokio_tungstenite::accept_hdr_async(raw_stream, copy_headers_callback)
        .await
        .expect("Error during the websocket handshake occurred");
    println!("WebSocket connection established: {}", addr);


    // Insert the write part of this peer to the peer map.
    let (tx, rx) = unbounded();

    let client = Client {
        socket_addr: addr,
        tx,
        uri: device_uri.unwrap(),
    };

    let temp_client = client.clone();
    let all_uri = Uri::from_static("all");

    peer_map.lock().await.push(client);

    let (outgoing, incoming) = ws_stream.split();

    let broadcast_incoming = incoming.try_for_each(|msg| {
        let timestamp = Utc::now();
        print!("{} - {} - URI: {}: ", timestamp.format("%Y-%m-%d - %H:%M"), temp_client.socket_addr, temp_client.uri);

        match msg {
            Message::Ping(_ping) => {
                println!("Ping");
            },
            Message::Pong(_pong) => {
                println!("Pong");
            },
            Message::Binary(_binary) => {
                println!("Binary");
            },
            Message::Text(text) => {
                println!("Client: {}, URI: {}, Message: {}", temp_client.socket_addr.clone(), temp_client.uri.clone(), text);

                let peers = block_on(peer_map.lock());

                if text.clone().eq("alert") {
                    let broadcast_recipients: Vec<&Client> = peers.iter().filter(|c| (c.uri == temp_client.uri || c.uri == all_uri)).collect();

                    let mes = Message::text(text.clone());
                    for recipient in broadcast_recipients {
                        recipient.tx.unbounded_send(mes.clone()).unwrap();
                    }
                }
            },
            Message::Close(_close) => {
                println!("Close");
            },
            Message::Frame(_frame) => {
                println!("Frame");
            }
        }

        future::ok(())
    });

    let receive_from_others = rx.map(Ok).forward(outgoing);

    pin_mut!(broadcast_incoming, receive_from_others);
    future::select(broadcast_incoming, receive_from_others).await;

    println!("{} disconnected", &addr);
    let index = peer_map.lock().await.iter().position(|c| c.socket_addr == addr).unwrap();
    peer_map.lock().await.remove(index);
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    dotenv().ok();

    let server_address = env::var("SERVER_ADDRESS").expect("SERVER_ADDRESS is not set in .env file");
    let server_port = env::var("SERVER_PORT").expect("SERVER_PORT is not set in .env file");

    let mut server_address = server_address;
    server_address.push(':');
    server_address.push_str(&server_port);


    // ---------- Global used variables
    let state = PeerMap::new(Mutex::new(Vec::new()));


    // ---------- Websocket starting and loop
    // Create the event loop and TCP listener we'll accept connections on.
    let try_socket = TcpListener::bind(&server_address).await;
    let listener = try_socket.expect("Failed to bind");

    println!("Listening on: {}", server_address);

    // Let's spawn the handling of each connection in a separate task.
    while let Ok((stream, socket_address)) = listener.accept().await {
        tokio::spawn(handle_connection(state.clone(), stream, socket_address));
    }

    Ok(())
}
