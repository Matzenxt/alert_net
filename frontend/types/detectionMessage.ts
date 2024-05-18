import {Device} from "~/types/device";

export class DetectionMessage {
  device: Device;
  source: string;

  constructor(device: Device, source: string) {
    this.device = device;
    this.source = source;
  }
}
