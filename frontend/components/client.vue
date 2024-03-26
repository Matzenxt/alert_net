<script setup lang="ts">
  import type {ClientOut} from "~/types/ClientOut";

  const props = defineProps({
    client: {type: Object as PropType<ClientOut>, required: true},
  });
</script>

<template>
  <v-card>
    <v-card-title>device</v-card-title>
    <v-card-subtitle>Addr: {{ client.socket_addr }} - Bereich: {{ client.uri }}</v-card-subtitle>
    <v-card-text>bild</v-card-text>
    <v-card-text>Status: ?</v-card-text>
  </v-card>
</template>

<style scoped>

</style>