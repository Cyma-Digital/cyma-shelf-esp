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
void checkAndWriteToFile();
String read(String filename);
void write(const ArduinoJson6200_F1::JsonVariantConst& json);


const char* SSID = "StMarche";
const char* password = NULL;
const char* jsonString = "{\"config\":{\"shelfs\":1,\"pixels\":40,\"colors\":[{\"name\":\"Branco\",\"value\":\"#ffffff\"},{\"name\":\"Desligado\",\"value\":\"#000000\"},{\"name\":\"Cinza\",\"value\":\"#808080\"},{\"name\":\"Vermelho\",\"value\":\"#ff0000\"},{\"name\":\"Verde\",\"value\":\"#00ff00\"},{\"name\":\"Azul\",\"value\":\"#0000ff\"},{\"name\":\"Amarelo\",\"value\":\"#ffff00\"},{\"name\":\"Laranja\",\"value\":\"#ffa500\"},{\"name\":\"Rosa\",\"value\":\"#ffc0cb\"},{\"name\":\"Roxo\",\"value\":\"#800080\"},{\"name\":\"Azul Claro\",\"value\":\"#0779bf\"}]},\"shelfs\":[{\"shelfIndex\":0,\"segmentsNumber\":1,\"segments\":[]}]}";



#define LED_PIN 22

WiFiClient client;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(9090);

#define NUM_LEDS 50
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
  
  checkAndWriteToFile();
}

void loop() {

    server.handleClient();
    webSocket.loop();
  
    FastLED.show();
}

void handleRoot() {

  Serial.println("Handle Root");

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

    // Send data to Front

    size_t len;
    DynamicJsonDocument data(2048);

    String dataFile = read("/data.txt");

    DeserializationError error = deserializeJson(data, dataFile);

    // Check for any errors during deserialization
    if (error) {
      Serial.print("Deserialization error: ");
      Serial.println(error.c_str());
      return;
    }

    len = measureJson(data);
    char jsonToSend[len];
    // serializeJson(data, Serial);
    serializeJson(data, jsonToSend, len + 1);
    webSocket.sendTXT(num, jsonToSend, strlen(jsonToSend));

  }
  break;
  case WStype_TEXT:
  {
    USE_SERIAL.printf("[%u] From Vue: %s\n", num, payload);

    const uint8_t size = JSON_OBJECT_SIZE(5);
    StaticJsonDocument<2048> json;
    DeserializationError err = deserializeJson(json, payload);

    if (err) {
        USE_SERIAL.printf("deserializeJson() failed with code ");
        USE_SERIAL.printf(err.c_str());
        return;
    }

    // serializeJson(json, Serial);

    write(json);

    // int segments = json["segmentsNumber"].as<int>();

    // USE_SERIAL.printf("segments: %d\n", segments);
    // USE_SERIAL.printf("segment count: %d\n", json["segments"].size());

    // int iIndex = 0;

    // for (int i=0; i<json["segments"].size(); i++){

    //   CRGB segColor = CRGB(json["segments"][i]["color"][0], json["segments"][i]["color"][1], json["segments"][i]["color"][2]);
    //   int segmentSize = i == json["segments"].size() - 1 ? 50 : json["segments"][i]["size"];

    //   for (int p=iIndex; p<segmentSize; p++){
    //     leds[p] = segColor;
    //   }
      
    //   iIndex = segmentSize;

    // }


  }
  break;
  default:
    break;
  }
}


/**
 * The function checks if a file exists and contains data, and if not, writes default data to it.
 * 
 * @return nothing (void).
 */
void checkAndWriteToFile(){
 
 String fileContent;

 SPIFFS.begin();
 File file = SPIFFS.open("/data.txt", "r");
 if (!file){
    Serial.println("There was an error opening the file for writing");
    return;
  }

  int fileSize = file.size();
  Serial.print("[*] File size: ");
  Serial.print(fileSize);
  Serial.println(" bytes");

  if(file.size() == 0){

    File file = SPIFFS.open("/data.txt", "w");
    Serial.println("[!] Blank file");
    if(file.print(jsonString)){
        Serial.println("[+] Default data written");
        file.close();

        SPIFFS.begin();
        File fileData = SPIFFS.open("/data.txt", "r");
        int len = fileData.size();

        char buffer[len];

        if(!fileData){
          Serial.println("[ERROR] Failed to open file for reading");
          return;

        }
  
        while(fileData.available()){
          // fileData.readBytes(buffer, len);
          fileContent += (char)fileData.read();
        }

        // for (int i = 0; i < len; i++){
        //   Serial.print(buffer[i]);
        // }
        Serial.println("\n[+] File Content:");
        Serial.println(fileContent);
        Serial.println();

        fileData.close();
        return;

    } else {
        Serial.println("[+] Default data write failed");
        return;
    }

  }

  file.close();

  Serial.println("[!] File already exists and contains data");
  return;

}


/**
 * This function reads the content of a file with a given filename from the SPIFFS file system in a
 * ESP32 or ESP8266 microcontroller.
 * 
 * @param filename The name of the file that needs to be read.
 * 
 * @return the content of the file with the given filename as a string. If the file cannot be opened
 * for reading, the function returns the string "None".
 */
String read(String filename) {
  String fileContent;

  SPIFFS.begin();
  File file = FFat.open(filename, "r");

  if(!file){
    Serial.println("[ERROR] Failed to open file for reading");
    return "None";
  }

  while(file.available()){
    fileContent += (char)file.read();
  }
  file.close();

  return fileContent;

}


/**
 * The function writes a JSON object to a file named "data.txt" on an SD card using the
 * ArduinoJson6200_F1 library.
 * 
 * @param json A constant reference to a JSON variant object from the ArduinoJson6200_F1 library.
 * 
 * @return The function does not have a return type specified, so it does not explicitly return
 * anything. However, it contains several return statements that will exit the function early if
 * certain conditions are met.
 */
void write(const ArduinoJson6200_F1::JsonVariantConst& json){
  Serial.println("\n[*] Saving");
  serializeJson(json, Serial);


  Serial.println("\n[*] Opening File");

  File file = FFat.open("/data.txt", "w");

  if(!file){
    Serial.println("[ERROR] There was an error opening the file for writing");
    return;
  }
  file.close();

  file = FFat.open("/data.txt", "w");

  if (!file) {
    Serial.println("There was an error reopening the file for writing");
    return;
  }


  if(serializeJson(json, file) == 0){
    Serial.println("[ERROR] Failed to write JSON to the file");
    return;
  }

  Serial.println("\n[+] File updated successfully");
  file.close();

}
