export class Device {
  id: number;
  uuid: string;
  description: string;
  area: string;

  constructor(id: number, uuid: string, description: string, area: string) {
    this.id = id;
    this.uuid = uuid;
    this.description = description;
    this.area = area;
  }
}
