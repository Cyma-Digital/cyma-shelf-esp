#include <Arduino.h>

#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFi.h>

#include <WebSocketsServer.h>

#include <SPIFFS.h>
#include <FFat.h>

#define USE_SERIAL Serial

void handleRoot();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleNotFound();
String htmlPage();
void startConfigWebpage();

const char* SSID = "StMarche";
const char* password = NULL;

uint8_t builtinLed = 5;  


WiFiClient client;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(9090);


void setup() {
  Serial.begin(115200);

  pinMode(builtinLed, OUTPUT);
  

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

  while (true)
  {
    server.handleClient();
    webSocket.loop();
  }
  

}

void loop() {
  digitalWrite(builtinLed, HIGH);
  delay(500);
  digitalWrite(builtinLed, LOW);
  delay(500);
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
  }
  break;
  default:
    break;
  }
}

