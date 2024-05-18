<script setup lang="ts">
  import {Device} from "~/types/device";
  import {DetectionMessage} from "~/types/detectionMessage";

  let ws = new WebSocket('ws://192.168.0.76:3000/laden');

  ws.onmessage = (event) => {
    console.log("Message");
    console.log(event.data);
    console.log("Type: " + event.type);
  };

  ws.onopen = (event) => {
    const message: string = "Here's some text that the server is urgently awaiting!";
    ws.send(message);
  };

  function detection() {
    if (ws.OPEN) {
      console.log("open test");

      const device: Device = new Device(1, "68246bfd-cafb-4e23-98f4-db1cd6d296a7", "UI Test", "laden");
      const detectionMessage: DetectionMessage = new DetectionMessage(device, "Motion");
      const jsonString: string = JSON.stringify(detectionMessage);

      ws.send(jsonString);
    }
  }


  function close() {
    ws.close(1000, "Exit");
  }

  function open() {
    ws = new WebSocket('ws://192.168.0.76:3000/laden');
  }
</script>

<template>
  <v-card>
    <v-card-title>Test Alerts</v-card-title>

    <v-divider/>

    <v-card-actions>
      <v-btn
          @click="detection();"
      >Detection</v-btn>

      <v-btn
          @click="close();"
      >Close</v-btn>

      <v-btn
          @click="open();"
      >Open</v-btn>
    </v-card-actions>
  </v-card>

</template>

<style scoped>

</style>
