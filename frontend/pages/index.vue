<script setup lang="ts">
    import type { Ref } from 'vue';
    import type {ClientOut} from "~/types/ClientOut";
    import {useIntervalFn} from "@vueuse/shared";

    let messages: Ref<Array<String>> = ref<Array<String>>([]);
    let clients: Ref<Array<ClientOut>> = ref<Array<ClientOut>>([]);


    let ws = new WebSocket('ws://192.168.0.76:3000/ui');
    
    ws.onmessage = (event) => {
        console.log("Message");
        console.log(event.data);
        messages.value.push(event.data);
        console.log("Type: " + event.type);

        clients.value = JSON.parse(event.data);
    };

    ws.onopen = (event) => {
      const message: string = "Here's some text that the server is urgently awaiting!";
        ws.send(message);
    };

    const {pause, resume, isActive} = useIntervalFn(getClients, 5000);

    function getClients() {
      if (ws.OPEN) {
        const message: string = "Get Clients";
        ws.send(message);
      }
    }

    function test() {
        if (ws.OPEN) {
          const message: string = "Hello from ui";
          ws.send(message);
        }
    }

    function close() {
      ws.close(1000, "Exit");
    }

    function open() {
      ws = new WebSocket('ws://192.168.0.76:3000/all');
    }
</script>

<template>
  <v-card max-height="500" height="500" min-height="500">
    <v-card-title>Dashboard</v-card-title>
    <v-divider/>
    <v-card-text>

      <Client
          v-for="client in clients" :key="client.socket_addr"
          v-bind:client="client"
      />

    </v-card-text>
    <v-divider/>
    <v-card-actions>
      <v-btn
          @click="test();"
      >send</v-btn>

      <v-btn
          @click="close();"
      >Close</v-btn>

      <v-btn
          @click="open();"
      >Open</v-btn>

      <v-spacer/>

      <v-btn
          @click="getClients();"
      >Get Clients</v-btn>
    </v-card-actions>

  </v-card>

  <v-card height="300">
    <v-card-text>
      <v-virtual-scroll
          :height="250"
          :items="messages"
      >
        <template v-slot:default="{ item }">
          {{ item }}
        </template>
      </v-virtual-scroll>
    </v-card-text>
  </v-card>

  <DetectionTest/>


</template>

<style scoped>

</style>
