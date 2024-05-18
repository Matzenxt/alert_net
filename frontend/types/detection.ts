import {Device} from "~/types/device";

export class Detection {
  id: number;
  device: Device;
  source: string;
  timestamp: string;

  constructor(id: number, device: Device, source: string, timestamp: string) {
    this.id = id;
    this.device = device;
    this.source = source;
    this.timestamp = timestamp;
  }
}
