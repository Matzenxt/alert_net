export class ClientOut {
  socket_addr: string;
  uri: string;

  constructor(socket_addr: string, uri: string) {
    this.socket_addr = socket_addr;
    this.uri = uri;
  }
}