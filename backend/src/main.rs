use std::{
    env, error::Error, net::SocketAddr, sync::{Arc, Mutex}
};

use futures_channel::mpsc::{unbounded, UnboundedSender};
use futures_util::{future, pin_mut, stream::TryStreamExt, StreamExt};

use serde::{Deserialize, Serialize};
use sqlx::Connection;
use tokio::net::{TcpListener, TcpStream};
use tokio_tungstenite::tungstenite::protocol::Message;
use tungstenite::{handshake::server::{ErrorResponse, Request, Response}, http::Uri};

#[derive(Clone)]
struct Client {
    socket_addr: SocketAddr,
    tx: Tx,
    uri: Uri,
}

#[derive(Clone, Deserialize, Serialize)]
struct ClientOut {
    socket_addr: String,
    uri: String,
}

#[derive(Deserialize, Serialize, Clone, Copy)]
struct Alert{
    led: bool,
    noise: bool,
}

type Tx = UnboundedSender<Message>;
type PeerMap = Arc<Mutex<Vec<Client>>>;

async fn handle_connection(peer_map: PeerMap, raw_stream: TcpStream, addr: SocketAddr) {
    println!("Incoming TCP connection from: {}", addr);

    let mut test: Option<Uri> = None;
    let copy_headers_callback = |request: &Request, response: Response| -> Result<Response, ErrorResponse> {
        let temp = Request::uri(request);
        println!("URI: {}", temp);

        test = Some(temp.clone());

        for (name, value) in request.headers().iter() {
            println!("Name: {}, value: {}", name.to_string(), value.to_str().expect("expected a value"));
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
        tx: tx,
        uri: test.unwrap(),
    };

    let temp_client = client.clone();

    peer_map.lock().unwrap().push(client);

    let (outgoing, incoming) = ws_stream.split();

    let broadcast_incoming = incoming.try_for_each(|msg| {
        //println!("Received a message from {}: {}", addr, msg.to_text().unwrap());

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
                println!("Client: {}, URI: {},  Message: {}", temp_client.socket_addr.clone(), temp_client.uri.clone(), text);

                let ui_uri = Uri::from_static("/ui");

                let peers = peer_map.lock().unwrap();
                let ui_recipients: Vec<&Client> = peers.iter().filter(|c| (c.uri == ui_uri)).collect();

                let mes = Message::text(text.clone());
                for recipient in ui_recipients {
                    recipient.tx.unbounded_send(mes.clone()).unwrap();
                }

                if text.clone().eq("Bewegung") {
                    let broadcast_recipients: Vec<&Client> = peers.iter().filter(|c| (c.uri == temp_client.uri)).collect();

                    let alert = Alert {
                        led: true,
                        noise: true,
                    };

                    let temp_msg = Message::text(serde_json::to_string(&alert).unwrap());

                    let mes = Message::text(text.clone());
                    for recipient in broadcast_recipients {
                        recipient.tx.unbounded_send(mes.clone()).unwrap();
                        recipient.tx.unbounded_send(temp_msg.clone()).unwrap();
                    }
                }

                if text.eq("Get Clients") {
                    let clients: Vec<&Client> = peers.iter().filter(|_c| true).collect();

                    let mut clients_out: Vec<ClientOut> = Vec::new();

                    for client in clients {
                        clients_out.push(ClientOut {
                            socket_addr: client.socket_addr.to_string(),
                            uri: client.uri.to_string(),
                        });
                    }

                    let clients_message = Message::text(serde_json::to_string(&clients_out).unwrap());
                    let broadcast_recipient: Vec<&Client> = peers.iter().filter(|c| (c.uri == temp_client.uri)).collect();
                    temp_client.tx.unbounded_send(clients_message).unwrap();
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
    let temp_addr = addr.clone();
    let index = peer_map.lock().unwrap().iter().position(|c| c.socket_addr == temp_addr).unwrap();
    peer_map.lock().unwrap().remove(index);
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    //let addr: String = env::args().nth(1).unwrap_or_else(|| "192.168.123.160:3000".to_string());
    let addr: String = env::args().nth(1).unwrap_or_else(|| "192.168.0.88:3000".to_string());

    let db_url = "postgres://server:server@localhost:5432/alert_net";
    let pool = sqlx::postgres::PgPool::connect(db_url).await?;

    sqlx::migrate!("./../migrations").run(&pool).await?;

    let state = PeerMap::new(Mutex::new(Vec::new()));

    // Create the event loop and TCP listener we'll accept connections on.
    let try_socket = TcpListener::bind(&addr).await;
    let listener = try_socket.expect("Failed to bind");
    println!("Listening on: {}", addr);

    // Let's spawn the handling of each connection in a separate task.
    while let Ok((stream, addr)) = listener.accept().await {
        tokio::spawn(handle_connection(state.clone(), stream, addr));
    }

    Ok(())
}
