#include <FS.h>
#include <SPI.h>

#include <Arduino.h>
#include <ArduinoJson.h>

// Audio setup block
#include "DFRobotDFPlayerMini.h"

#if (defined(ARDUINO_AVR_UNO) || defined(ESP8266))   // Using a soft serial port
  #include <SoftwareSerial.h>
  SoftwareSerial softSerial(/*rx =*/D5, /*tx =*/D6);
  #define FPSerial softSerial
#else
  #define FPSerial Serial1
#endif

DFRobotDFPlayerMini myDFPlayer;
void printAudioDetail(uint8_t type, int value);

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsClient.h>

// Wifi:
//WiFiManager wifi_manager;
// select which pin will trigger the configuration portal when set to LOW
#define TRIGGER_PIN D2
int timeout = 120; // seconds to run for

// Websocket:
WebSocketsClient webSocket;

unsigned long messageInterval = 5000;
unsigned long lastUpdate = millis();
bool connected = false;

// Json:
JsonDocument doc;
bool led = false;
bool speaker = true;
bool reboot = false;

// Motion:
int motionPin = D1;
int pirState = LOW;

// Config:
#define JSON_CONFIG_FILE "/config.json"
bool shouldSaveConfig = true;

// Server:
char server_ip[40] = "192.168.0.88";
uint16_t server_port = 3000;

// Device:
uint16_t device_id = 0;
char device_uuid[50] = "to_register";
char device_area[50] = "test";
char device_description[50] = "Gerät 1";
uint16_t device_speaker_volume = 15; // Speaker volume. Set between 0 and 30.

void saveConfigFile() {
  Serial.println(F("Saving configuration..."));

  JsonDocument config;
  config["server_ip"] = server_ip;
  config["server_port"] = server_port;
  config["device"]["id"] = device_id;
  config["device"]["uuid"] = device_uuid;
  config["device"]["area"] = device_area;
  config["device"]["description"] = device_description;
  config["device"]["speaker_volume"] = device_speaker_volume;

  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing.");
  }

  serializeJsonPretty(config, Serial);
  if (serializeJsonPretty(config, configFile) == 0) {
    Serial.println(F("Failed to write config to file"));
  }

  Serial.println(F("Saved config file."));
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
          Serial.println("Parsed config file");

          strcpy(server_ip, config["server_ip"]);
          server_port = config["server_port"].as<uint16_t>();
          device_id = config["device"]["id"].as<uint16_t>();
          strcpy(device_uuid, config["device"]["uuid"]);
          strcpy(device_area, config["device"]["area"]);
          strcpy(device_description, config["device"]["description"]);
          device_speaker_volume = config["device"]["speaker_volume"].as<uint16_t>();

          return true;
        } else {
          Serial.println("Failed to parse config file");
        }

        configFile.close();
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

void initAudio() {
  #if (defined ESP32)
    FPSerial.begin(9600, SERIAL_8N1, /*rx =*/D3, /*tx =*/D2);
  #else
    FPSerial.begin(9600);
  #endif

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  if (!myDFPlayer.begin(FPSerial, /*isACK = */true, /*doReset = */false)) {  //Use serial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));

  int volume = 30;
  if (device_speaker_volume > 30) {
    volume = 30;
  } else if (device_speaker_volume < 0) {
    volume = 0;
  } else {
    volume = device_speaker_volume;
  }
  Serial.println(device_speaker_volume);
  Serial.println(volume);

  myDFPlayer.volume(volume);  //Set volume value. From 0 to 30
}

void printAudioDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void updateDeviceConfigFromServer(uint8_t * text) {
  JsonDocument config;
  DeserializationError error = deserializeJson(config, text);

  if (!error) {
    if (config.containsKey("uuid")) {
      Serial.println("Updated device config:");
      serializeJsonPretty(config, Serial);

      device_id = config["id"].as<uint16_t>();
      strcpy(device_uuid, config["uuid"]);
      strcpy(device_area, config["area"]);
      strcpy(device_description, config["description"]);

      saveConfigFile();
    }
  } else {
    Serial.println("Failed to parse config file");
  }
}

void alert(uint8_t * text) {
  JsonDocument alert;
  DeserializationError error = deserializeJson(alert, text);

  if (!error) {
    if (alert.containsKey("led")) {
      led = alert["led"].as<bool>();
      speaker = alert["speaker"].as<bool>();

      Serial.printf("LED: %u\n", led);
      Serial.printf("Speaker: %u\n", speaker);

      // actions:
      if (speaker == true) {
        myDFPlayer.play(1);
      }
    }
  } else {
    Serial.println("Failed to parse config file");
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

      // Check if divce is registered by server.
      Serial.print("Saved uuid: ");
      Serial.println(device_uuid);
      Serial.print("Res: ");
      Serial.println(strcmp(device_uuid, "to_register"));


      if (strcmp(device_uuid, "to_register") == 0) {
        Serial.println("Device is not registered by server.");
        Serial.println("Getting new uuid from server:");

        JsonDocument doc;
        doc["id"] = device_id;
        doc["uuid"] = "39472baa-7bcf-4d56-9829-f17bb5b543cd";
        doc["description"] = device_description;
        doc["area"] = device_area;

        String output = "";

        serializeJsonPretty(doc, output);

        webSocket.sendTXT(output);
      } else {
        Serial.println("Device already registered.");
      }
			break;
		case WStype_TEXT:
			Serial.printf("[WSc] get text: %s\n", payload);


      // Check for json messages
      updateDeviceConfigFromServer(payload);
      alert(payload);


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

void sendDetectionMessage(String source) {
  JsonDocument doc;

  doc["device"]["id"] = device_id;
  doc["device"]["uuid"] = device_uuid;
  doc["device"]["description"] = device_description;
  doc["device"]["area"] = device_area;
  doc["source"] = source;

  String output = "";
  serializeJsonPretty(doc, output);

  webSocket.sendTXT(output);
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
    Serial.println("Trigger pin for portal active.");
  }

  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config!"));
    forceConfig = true;
  }

  WiFi.mode(WIFI_STA);

  WiFiManager wifi_manager;
  wifi_manager.setSaveConfigCallback(saveConfigCallback);
  wifi_manager.setAPCallback(configModeCallback);

  // Custom elements:
  WiFiManagerParameter custom_server_ip("server_ip", "Server IP", server_ip, 40);
  char convertedPortValue[6];
  sprintf(convertedPortValue, "%d", server_port);
  WiFiManagerParameter custom_server_port("server_port", "Server Port", convertedPortValue, 7);
  WiFiManagerParameter custom_area("area", "Bereich", device_area, 50);
  WiFiManagerParameter custom_description("description", "Bezeichnung", device_description, 50);
  char convertedVolumeValue[2];
  sprintf(convertedVolumeValue, "%d", device_speaker_volume);
  WiFiManagerParameter custom_speaker_volume("15", "Lautstärke. Min: 0, Max: 30", convertedVolumeValue, 2);

  wifi_manager.addParameter(&custom_server_ip);
  wifi_manager.addParameter(&custom_server_port);
  wifi_manager.addParameter(&custom_area);
  wifi_manager.addParameter(&custom_description);
  wifi_manager.addParameter(&custom_speaker_volume);

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
  strncpy(device_area, custom_area.getValue(), sizeof(device_area));
  strncpy(device_description, custom_description.getValue(), sizeof(device_description));
  device_speaker_volume = (uint16_t)atol(custom_server_port.getValue());

  Serial.print("Server: ");
  Serial.println(server_ip);
  Serial.print("Port: ");
  Serial.println(server_port);
  Serial.print("ID: ");
  Serial.println(device_id);
  Serial.print("UUID: ");
  Serial.println(device_uuid);
  Serial.print("Bereich: ");
  Serial.println(device_area);
  Serial.print("Description: ");
  Serial.println(device_description);
  Serial.print("Speaker volume: ");
  Serial.println(device_speaker_volume);

  if (shouldSaveConfig) {
    saveConfigFile();
  }


  initAudio();

	// server address, port and URL
  char uri [52];
  strcpy(uri, "/");
  strcpy(uri, device_area);

	webSocket.begin(server_ip, server_port, uri);

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	//webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);

  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocket.enableHeartbeat(15000, 3000, 2);
}

void loop() {
  // Websocket part:
	webSocket.loop();

  if(digitalRead(motionPin) == HIGH) {
    if(pirState == LOW) {
      digitalWrite(LED_BUILTIN, LOW);

      Serial.println("Bewegung");
      sendDetectionMessage("Bewegung");

      pirState = HIGH;
    }
  } else {
    if(pirState == HIGH) {
      digitalWrite(LED_BUILTIN, HIGH);

      Serial.println("Nichts");
      pirState = LOW;
    }
  }

  if (myDFPlayer.available()) {
    printAudioDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
}


