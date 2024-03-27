#include <FS.h>
#include <SPI.h>

#include <Arduino.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>

// Wifi:
WiFiManager wifi_manager;
// select which pin will trigger the configuration portal when set to LOW
#define TRIGGER_PIN D6 
int timeout = 120; // seconds to run for

// Websocket:
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

// Config:
#define JSON_CONFIG_FILE "/config.json"
bool shouldSaveConfig = true;

char server_ip[40] = "192.168.0.88";
uint16_t server_port = 3000;
char area[50] = "test";
char description[50] = "Gerät 1";

void saveConfigFile() {
  Serial.println(F("Saving configuration..."));

  JsonDocument config;
  config["server_ip"] = server_ip;
  config["server_port"] = server_port;
  config["area"] = area;
  config["description"] = description;

  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing.");
  }

  serializeJsonPretty(config, Serial);
  if (serializeJsonPretty(config, configFile) == 0) {
    Serial.println(F("Failed to write config to file"));
  }

  configFile.close();
}

bool loadConfigFile() {
  //SPIFFS.format();

  Serial.println("Mounting file system.");

  if (SPIFFS.begin()) {
    Serial.println("Mounted file system");

    if (SPIFFS.exists(JSON_CONFIG_FILE)) {
      Serial.println("Reading config file.");

      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile) {
        Serial.println("Open config file.");
        JsonDocument config;
        DeserializationError error = deserializeJson(config, configFile);
        serializeJsonPretty(config, Serial);

        if (!error) {
          Serial.println("Parsing config file");

          strcpy(server_ip, config["server_ip"]);
          server_port = config["server_port"].as<uint16_t>();
          strcpy(area, config["area"]);
          strcpy(description, config["description"]);

          return true;
        } else {
          Serial.println("Failed to parse config file");
        }
      }
    }
  } else {
    Serial.println("Failed to mount file system");
  }

  return false;
}

void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered configruation mode");

  Serial.print(" Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print(" Config IP address: ");
  Serial.println(WiFi.softAPIP());
}

void wifi_config_init(bool forceConfig) {
  WiFi.mode(WIFI_STA);

  wifi_manager.setSaveConfigCallback(saveConfigCallback);
  wifi_manager.setAPCallback(configModeCallback);

  // Custom elements:
  WiFiManagerParameter custom_server_ip("server_ip", "Server IP", server_ip, 40);
  char convertedPortValue[6];
  sprintf(convertedPortValue, "%d", server_port);
  WiFiManagerParameter custom_server_port("server_port", "Server Port", convertedPortValue, 7);
  WiFiManagerParameter custom_area("area", "Bereich", area, 50);
  WiFiManagerParameter custom_description("description", "Bezeichnung", description, 50);

  wifi_manager.addParameter(&custom_server_ip);
  wifi_manager.addParameter(&custom_server_port);
  wifi_manager.addParameter(&custom_area);
  wifi_manager.addParameter(&custom_description);

  //wifi_manager.resetSettings();

  if (forceConfig) {
    if (!wifi_manager.startConfigPortal("AlertNet", "123456789")) {
      Serial.println("Failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
    }
  } else {
    if (!wifi_manager.autoConnect("AlertNet", "123456789")) {
            Serial.println("Failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
    }
  }

  Serial.println("");
  Serial.println("Wifi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  strncpy(server_ip, custom_server_ip.getValue(), sizeof(server_ip));
  server_port = (uint16_t)atol(custom_server_port.getValue());
  strncpy(area, custom_area.getValue(), sizeof(area));
  strncpy(description, custom_description.getValue(), sizeof(description));

  Serial.print("Server: ");
  Serial.println(server_ip);
  Serial.print("Port: ");
  Serial.println(server_port);
  Serial.print("Bereich: ");
  Serial.println(area);
  Serial.print("Description: ");
  Serial.println(description);

  if (shouldSaveConfig) {
    saveConfigFile();
  }
}

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
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  Serial.begin(115200);
	Serial.setDebugOutput(true);

	Serial.println();
	Serial.println();
	Serial.println();

	for(uint8_t t = 4; t > 0; t--) {
		Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
		Serial.flush();
		delay(1000);
	}

  bool forceConfig = false;

  if (digitalRead(TRIGGER_PIN) == LOW) {
    forceConfig = true;
  }

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config!"));
    forceConfig = true;
  }

  WiFi.mode(WIFI_STA);

  wifi_manager.setSaveConfigCallback(saveConfigCallback);
  wifi_manager.setAPCallback(configModeCallback);

  // Custom elements:
  WiFiManagerParameter custom_server_ip("server_ip", "Server IP", server_ip, 40);
  char convertedPortValue[6];
  sprintf(convertedPortValue, "%d", server_port);
  WiFiManagerParameter custom_server_port("server_port", "Server Port", convertedPortValue, 7);
  WiFiManagerParameter custom_area("area", "Bereich", area, 50);
  WiFiManagerParameter custom_description("description", "Bezeichnung", description, 50);

  wifi_manager.addParameter(&custom_server_ip);
  wifi_manager.addParameter(&custom_server_port);
  wifi_manager.addParameter(&custom_area);
  wifi_manager.addParameter(&custom_description);

  //wifi_manager.resetSettings();

  if (forceConfig) {
    if (!wifi_manager.startConfigPortal("AlertNet", "123456789")) {
      Serial.println("Failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
    }
  } else {
    if (!wifi_manager.autoConnect("AlertNet", "123456789")) {
            Serial.println("Failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
    }
  }

  Serial.println("");
  Serial.println("Wifi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  strncpy(server_ip, custom_server_ip.getValue(), sizeof(server_ip));
  server_port = (uint16_t)atol(custom_server_port.getValue());
  strncpy(area, custom_area.getValue(), sizeof(area));
  strncpy(description, custom_description.getValue(), sizeof(description));

  Serial.print("Server: ");
  Serial.println(server_ip);
  Serial.print("Port: ");
  Serial.println(server_port);
  Serial.print("Bereich: ");
  Serial.println(area);
  Serial.print("Description: ");
  Serial.println(description);

  if (shouldSaveConfig) {
    saveConfigFile();
  }


	// server address, port and URL
  char uri [52];
  strcpy(uri, "/");
  strcpy(uri, area);

	webSocket.begin(server_ip, server_port, uri);

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
  // Websocket part:
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
