#include <Arduino.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>


// Wifi and websocket block:
ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

unsigned long messageInterval = 5000;
unsigned long lastUpdate = millis();
bool connected = false;

// Json:
JsonDocument doc;
bool led = false;
bool noise = true;
bool reboot = false;

// Motion:
int motionPin = D1;
int pirState = LOW;


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[WSc] Disconnected!\n");

      connected = false;
			break;
		case WStype_CONNECTED:
			Serial.printf("[WSc] Connected to url: %s\n", payload);

      connected = true;

			// send message to server when Connected
			webSocket.sendTXT("Connected");
			break;
		case WStype_TEXT:
			Serial.printf("[WSc] get text: %s\n", payload);


      deserializeJson(doc, payload);

      led = doc["led"].as<bool>();
      noise = doc["noise"].as<bool>();
      reboot = doc["reboot"].as<bool>();

      Serial.printf("LED: %u\n", led);
      Serial.printf("Noise: %u\n", noise);
      Serial.printf("Reboot: %u\n", reboot);
    
      if (reboot == true) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);

        Serial.printf("Restarting esp in 5 seconds");
        webSocket.disconnect();
        delay(5000);
        ESP.restart();
      }
      
			// send message to server
      if(digitalRead(LED_BUILTIN == HIGH)) {
			  //webSocket.sendTXT("Licht");
      }
			break;
		case WStype_BIN:
			Serial.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);

			// send data to server
			// webSocket.sendBIN(payload, length);
			break;
    case WStype_PING:
      // pong will be send automatically
      Serial.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      Serial.printf("[WSc] get pong\n");

      if (digitalRead(LED_BUILTIN) == HIGH) {
        //digitalWrite(LED_BUILTIN, LOW);
      } else {
        //digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
      }

      break;
    }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

	Serial.setDebugOutput(true);

	Serial.println();
	Serial.println();
	Serial.println();

	for(uint8_t t = 4; t > 0; t--) {
		Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
		Serial.flush();
		delay(1000);
	}

	WiFiMulti.addAP("Privat-Wlan", "99842DA5D0");
	WiFiMulti.addAP("Privat W-LAN", "99842DA5D0");

	//WiFi.disconnect();
	while(WiFiMulti.run() != WL_CONNECTED) {
		delay(100);
	}

	// server address, port and URL
	//webSocket.begin("192.168.123.160", 3000, "/laden");
	webSocket.begin("192.168.0.88", 3000, "/kino");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	//webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
  webSocket.setExtraHeaders("room: Laden");

  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocket.enableHeartbeat(15000, 3000, 2);
}

int counter = 0;

void loop() {
	webSocket.loop();
  
  if (connected && lastUpdate + messageInterval < millis()) {
    Serial.println("Send demo message to server");
    webSocket.sendTXT("Demo maessage");
    counter = counter + 1;
    lastUpdate = millis();
  }

  if(connected && counter == 5) {
    Serial.println("Send Reboot Message");
    webSocket.sendTXT("Reboot");
    counter = counter + 1;
  }
}


