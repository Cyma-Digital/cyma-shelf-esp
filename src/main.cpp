#include <Arduino.h>
#include "FastLED.h"

#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFi.h>

#include <WebSocketsServer.h>

#include <SPIFFS.h>
#include <FFat.h>
#include <ArduinoJson.h>

#define USE_SERIAL Serial

FASTLED_USING_NAMESPACE

void handleRoot();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleNotFound();
String htmlPage();
void startConfigWebpage();

const char* SSID = "StMarche";
const char* password = NULL;

#define LED_PIN 22

WiFiClient client;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(9090);

#define NUM_LEDS  50 
CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);

  pinMode(5, OUTPUT);
  
  FastLED.addLeds<WS2811, LED_PIN, RGB>(leds, 0, NUM_LEDS); //define

  if (!FFat.begin(true)){
    Serial.println("Couldn't mount File System");
  }

  Serial.println("\n[+] Creating Access Point...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SSID, password);

  Serial.print("[+] AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());


  Serial.println("[+] Creating websocket server... ");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);


  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  // while (true)
  // {
  //   server.handleClient();
  //   webSocket.loop();
  // }
  

}

void loop() {

    server.handleClient();
    webSocket.loop();
  
    FastLED.show();

  // digitalWrite(builtinLed, HIGH);
  // delay(500);
  // digitalWrite(builtinLed, LOW);
  // delay(500);
}

void handleRoot() {

  Serial.println("Hanlder Root");

  SPIFFS.begin();
  File file = SPIFFS.open("/index.html.gz", "r");
  server.streamFile(file, "text/html");
  file.close();
  SPIFFS.end();
}

void handleNotFound() {
  Serial.println("Handle not Found");
  server.send(404,"text/plain", "404: Not found");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length){

  switch (type)
  {
  case WStype_DISCONNECTED:
      USE_SERIAL.printf("[%u] Disconnected!\n", num);
      break;
  
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
  }
  break;
  case WStype_TEXT:
  {
    USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

    const uint8_t size = JSON_OBJECT_SIZE(5);
    StaticJsonDocument<2048> json;
    DeserializationError err = deserializeJson(json, payload);

    if (err) {
        USE_SERIAL.printf("deserializeJson() failed with code ");
        USE_SERIAL.printf(err.c_str());
        return;
    }


    int segments = json["segmentsNumber"].as<int>();

    USE_SERIAL.printf("segments: %d\n", segments);
    USE_SERIAL.printf("segment count: %d\n", json["segments"].size());

    int iIndex = 0;

    for (int i=0; i<json["segments"].size(); i++){

      CRGB segColor = CRGB(json["segments"][i]["color"][0], json["segments"][i]["color"][1], json["segments"][i]["color"][2]);
      int segmentSize = i == json["segments"].size() - 1 ? 50 : json["segments"][i]["size"];

      for (int p=iIndex; p<segmentSize; p++){
        leds[p] = segColor;
      }
      
      iIndex = segmentSize;

    }


  }
  break;
  default:
    break;
  }
}

