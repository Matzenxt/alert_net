<script setup lang="ts">
    import type { Ref } from 'vue';
    import type {ClientOut} from "~/types/ClientOut";
    import {useIntervalFn} from "@vueuse/shared";

    let messages: Ref<Array<String>> = ref<Array<String>>([]);
    let clients: Ref<Array<ClientOut>> = ref<Array<ClientOut>>([]);


    let ws = new WebSocket('ws://192.168.0.88:3000/ui');
    
    ws.onmessage = (event) => {
        console.log("Message");
        console.log(event.data);
        messages.value.push(event.data);
        console.log("Type: " + event.type);

        clients.value = JSON.parse(event.data);

    };


    ws.onopen = (event) => {
        ws.send("Here's some text that the server is urgently awaiting!");
    };

    const {pause, resume, isActive} = useIntervalFn(getClients, 5000);

    function getClients() {
      if (ws.OPEN) {
        ws.send("Get Clients");
      }
    }

    function test() {
        if (ws.OPEN) {
            ws.send("Hello from ui");
        }
    }

    function close() {
      ws.close(1000, "Exit");
    }

    function open() {
      ws = new WebSocket('ws://192.168.0.88:3000/all');
    }
</script>

<template>
  <v-card max-height="800" height="800" min-height="800">
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

  <v-container>
    <v-row v-for="(message, index) in messages" :key="index">
      {{ message }}
    </v-row>
  </v-container>


</template>

<style scoped>

</style>
