mod message;
mod common;
mod database;

use std::{env, thread};
use std::error::Error;
use std::net::SocketAddr;
use std::sync::{Arc};
use dotenv::dotenv;
use futures::executor::block_on;

use futures_channel::mpsc::{unbounded, UnboundedSender};
use futures_util::{future, pin_mut, stream::TryStreamExt, StreamExt};

use serde::{Deserialize, Serialize};
use sqlx::{Acquire, Connection, Pool, Postgres};
use sqlx::postgres::PgPoolOptions;
use tokio::net::{TcpListener, TcpStream};
use tokio::sync::Mutex;
use tokio_tungstenite::tungstenite::protocol::Message;
use tungstenite::{handshake::server::{ErrorResponse, Request, Response}, http::Uri};
use uuid::Uuid;
use crate::common::models::device::Device;
use crate::database::Database;
use crate::message::receive::detection::DetectionMessage;
use crate::message::send::alert::Alert;

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

enum MessageAction {
    Register((Device, SocketAddr)),
    Detection(DetectionMessage),
    Test(String),
}

type Tx = UnboundedSender<Message>;
type PeerMap = Arc<Mutex<Vec<Client>>>;

async fn handle_connection(peer_map: PeerMap, raw_stream: TcpStream, addr: SocketAddr, tx_test: tokio::sync::mpsc::UnboundedSender<MessageAction>) {
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
    let all_uri = Uri::from_static("all");

    peer_map.lock().await.push(client);

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
                println!("Client: {}, URI: {}, Message: {}", temp_client.socket_addr.clone(), temp_client.uri.clone(), text);

                let ui_uri = Uri::from_static("/ui");

                let peers = block_on(peer_map.lock());
                let ui_recipients: Vec<&Client> = peers.iter().filter(|c| (c.uri == ui_uri)).collect();

                let mes = Message::text(text.clone());
                for recipient in ui_recipients {
                    recipient.tx.unbounded_send(mes.clone()).unwrap();
                }

                // TODO: Remove later on
                tx_test.send(MessageAction::Test(text.clone()));

                let temp: Result<Device, _> = serde_json::from_str(&*text);
                match temp {
                    Ok(dev) => {
                        println!("register code ausführen");
                        let d = Device {
                          uuid: Uuid::new_v4(),
                            ..dev
                        };

                        tx_test.send(MessageAction::Register((d.clone(), temp_client.socket_addr.clone())));
                    }
                    Err(_) => {}
                }

                let temp: Result<DetectionMessage, _> = serde_json::from_str(&*text);
                match temp {
                    Ok(detection_message) => {
                        println!("Detection");

                        tx_test.send(MessageAction::Detection(detection_message));

                        let broadcast_recipients: Vec<&Client> = peers.iter().filter(|c| (c.uri == temp_client.uri || c.uri == all_uri)).collect();

                        let alert = Alert {
                            led: true,
                            speaker: true,
                        };

                        let temp_msg = Message::text(serde_json::to_string(&alert).unwrap());

                        for recipient in broadcast_recipients {
                            recipient.tx.unbounded_send(temp_msg.clone()).unwrap();
                        }
                    }
                    Err(_) => {}
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
    let index = peer_map.lock().await.iter().position(|c| c.socket_addr == temp_addr).unwrap();
    peer_map.lock().await.remove(index);
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    dotenv().ok();

    let server_address = env::var("SERVER_ADDRESS").expect("SERVER_ADDRESS is not set in .env file");
    let server_port = env::var("SERVER_PORT").expect("SERVER_PORT is not set in .env file");
    let database = env::var("DATABASE_URL").expect("DATABASE is not set in .env file");

    let mut server_address = server_address;
    server_address.push_str(":");
    server_address.push_str(&*server_port);

    let db_url = database.as_str();
    let pool = PgPoolOptions::new()
        .min_connections(10)
        .connect(db_url).await?;

    sqlx::migrate!("./../migrations").run(&pool).await?;



    // ---------- Global used variables
    let p: Arc<Mutex<Pool<Postgres>>> = Arc::new(Mutex::new(pool));
    let state = PeerMap::new(Mutex::new(Vec::new()));



    // ---------- Internal message handler section
    let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel();
    let peer_map = state.clone();

    println!("Starting internal listener");
    tokio::spawn(async move {
        loop {
            let received = rx.recv().await.unwrap();

            match received {
                MessageAction::Register((device, socket_addr)) => {
                    println!("Register new device in database");

                    let t = p.lock().await;
                    let res = device.insert(&t).await;
                    println!("RES: {:#?}", res);
                    let dev = res.unwrap();

                    let peers = block_on(peer_map.lock());
                    let receiver_devices: Vec<&Client > = peers.iter().filter(|c| (c.socket_addr == socket_addr)).collect();
                    let receiver_device: &Client = receiver_devices.first().unwrap();

                    let clients_message = Message::text(serde_json::to_string(&dev).unwrap());
                    receiver_device.tx.unbounded_send(clients_message).unwrap();
                },
                MessageAction::Detection(detectionM_message) => {
                    let pool = p.lock().await;
                    let result = detectionM_message.insert(&pool).await;
                    println!("RES: {:#?}", result);
                },
                MessageAction::Test(msg) => {
                    //println!("Test msg to message handler. MSG: {}", msg);
                },
            }
        }
    });



    // ---------- Websocket starting and loop
    // Create the event loop and TCP listener we'll accept connections on.
    let try_socket = TcpListener::bind(&server_address).await;
    let listener = try_socket.expect("Failed to bind");

    println!("Listening on: {}", server_address);

    // Let's spawn the handling of each connection in a separate task.
    while let Ok((stream, socket_address)) = listener.accept().await {
        tokio::spawn(handle_connection(state.clone(), stream, socket_address, tx.clone()));
    }



    Ok(())
}
