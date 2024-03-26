#include <Arduino.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>



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

// Motion:
int motionPin = D1;
int pirState = LOW;

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

  myDFPlayer.volume(30);  //Set volume value. From 0 to 30
  myDFPlayer.play(1);  //Play the first mp3
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
      Serial.printf("LED: %u\n", led);
      Serial.printf("Noise: %u\n", noise);
    
      if (noise == true) {
        myDFPlayer.play(1);
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
        digitalWrite(LED_BUILTIN, LOW);
      } else {
        //digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
      }

      break;
    }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);


  initAudio();


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
	webSocket.begin("192.168.123.160", 3000, "/laden");
	//webSocket.begin("192.168.0.88", 3000, "/kino");

	// event handler
	webSocket.onEvent(webSocketEvent);

	// use HTTP Basic Authorization this is optional remove if not needed
	//webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
  webSocket.setExtraHeaders("room: Laden");
  webSocket.setExtraHeaders("position: Alarm");
  
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocket.enableHeartbeat(15000, 3000, 2);
}

void loop() {
	webSocket.loop();
  
  if (connected && lastUpdate + messageInterval < millis()) {
    //Serial.println("Test");
    //webSocket.sendTXT("asdf");
    lastUpdate = millis();
  }

  if(digitalRead(motionPin) == HIGH) {
    if(pirState == LOW) {
      digitalWrite(LED_BUILTIN, LOW);

      Serial.println("Bewegung");
      //myDFPlayer.play(1);

      webSocket.sendTXT("Bewegung");

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

