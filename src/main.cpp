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
void handleCategory();
bool checkTerminalColor(String colorFromTerminal);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleNotFound();
String htmlPage();
void startConfigWebpage();
void checkAndWriteToFile();
String read(String filename);
void write(const ArduinoJson::JsonVariantConst& json);
void setLEDStripProperties();
void clearLEDStripToApplyConfig(int shelfid);
CRGB parseColorString(String colorString);
DynamicJsonDocument readFileAndConvertoArduinoJson(String filename);
void dumpJsonArray(const JsonArray& jsonArray);



const char* SSID = "StMarche";
const char* password = NULL;
const char* jsonString = "{\"config\":{\"shelfs\":1,\"pixels\":40,\"colors\":[{\"name\":\"Branco\",\"value\":\"#ffffff\"},{\"name\":\"Desligado\",\"value\":\"#000000\"},{\"name\":\"Cinza\",\"value\":\"#808080\"},{\"name\":\"Vermelho\",\"value\":\"#ff0000\"},{\"name\":\"Verde\",\"value\":\"#00ff00\"},{\"name\":\"Azul\",\"value\":\"#0000ff\"},{\"name\":\"Amarelo\",\"value\":\"#ffff00\"},{\"name\":\"Laranja\",\"value\":\"#ffa500\"},{\"name\":\"Rosa\",\"value\":\"#ffc0cb\"},{\"name\":\"Roxo\",\"value\":\"#800080\"},{\"name\":\"Azul Claro\",\"value\":\"#0779bf\"}]},\"shelfs\":[{\"shelfIndex\":0,\"segmentsNumber\":1,\"segments\":[]}]}";

#define LED_PIN_1 2
#define LED_PIN_2 4
#define LED_PIN_3 12
#define LED_PIN_4 14
#define LED_PIN_5 15
#define LED_PIN_6 26
#define LED_PIN_7 25
#define LED_PIN_8 18

// #define CONNECTION_MODE "WiFi"
#define CONNECTION_MODE "noWiFi"

WiFiClient client;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(9090);

#define NUM_LEDS 100
CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGB leds3[NUM_LEDS];
CRGB leds4[NUM_LEDS];
CRGB leds5[NUM_LEDS];
CRGB leds6[NUM_LEDS];
CRGB leds7[NUM_LEDS];
CRGB leds8[NUM_LEDS];

void setup() {
  Serial.begin(115200);

  pinMode(5, OUTPUT);
  
  FastLED.addLeds<WS2812, LED_PIN_1, RGB>(leds1, 0, NUM_LEDS); //define
  FastLED.addLeds<WS2812, LED_PIN_2, RGB>(leds2, 0, NUM_LEDS); //define
  FastLED.addLeds<WS2812, LED_PIN_3, RGB>(leds3, 0, NUM_LEDS); //define
  FastLED.addLeds<WS2812, LED_PIN_4, RGB>(leds4, 0, NUM_LEDS); //define
  FastLED.addLeds<WS2812, LED_PIN_5, RGB>(leds5, 0, NUM_LEDS); //define
  FastLED.addLeds<WS2812, LED_PIN_6, RGB>(leds6, 0, NUM_LEDS); //define
  FastLED.addLeds<WS2812, LED_PIN_7, RGB>(leds7, 0, NUM_LEDS); //define
  FastLED.addLeds<WS2812, LED_PIN_8, RGB>(leds8, 0, NUM_LEDS); //define


  if (!FFat.begin(true)){
    Serial.println("Couldn't mount File System");
  }


  if(CONNECTION_MODE == "WiFi"){
    Serial.println("\n[+] Creating Access Point...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(SSID, password);

    Serial.print("[+] AP Created with IP Gateway ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("\n[+] Connecting on WiFi network...");
    WiFi.begin("DisplayHNK-Net", "cyma102030");
  }




  Serial.println("[+] Creating websocket server... ");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);


  server.on("/", handleRoot);
  server.on("/category", handleCategory);
  server.onNotFound(handleNotFound);
  server.begin();
  
  checkAndWriteToFile();
  
  setLEDStripProperties();
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

void handleCategory(){
  Serial.println("\nHandler Category");

  
  if(server.hasArg("plain") == false){
    return;
  }

  
  DynamicJsonDocument json(2048);
  String body = server.arg("plain");
  String response;
  DeserializationError error = deserializeJson(json, body);
  if (error) {
    Serial.println("Error");
    return;
  }


  serializeJson(json, Serial);
  USE_SERIAL.println();
  

  String colorFromTerminal = json["color"].as<String>();

  // TODO: Go through the segments and check if they are in the color of the category

 


  DynamicJsonDocument jsonFile = readFileAndConvertoArduinoJson("/data.txt");

  JsonArray shelfsArray = jsonFile["shelfs"].as<JsonArray>();
  dumpJsonArray(shelfsArray);

  
  for (JsonVariant shelf : shelfsArray) {
    JsonArray shelfsSegments = shelf["segments"].as<JsonArray>();
    dumpJsonArray(shelfsSegments);
  }

  
  if(checkTerminalColor(colorFromTerminal)){
    response = "{\"gondola\":\"show category\"}";
  } else {
    response = "{\"gondola\":\"not selected\"}";
  }



  server.send(200, "application/json", response);

}


bool checkTerminalColor(String colorFromTerminal){
  
  String dataFile = read("/data.txt");

  DynamicJsonDocument json(2048);
  DeserializationError err = deserializeJson(json, dataFile);
  if (err) {
    Serial.println("Error");
    return false;
  }

  
  JsonArray colors = json["config"]["colors"].as<JsonArray>();


  for (size_t i = 0; i < colors.size(); i++){
    JsonObject color = colors[i].as<JsonObject>();

    String colorValue = color["value"].as<String>();

    if(colorValue == colorFromTerminal) {
      USE_SERIAL.println("Color finded");
      return true;
    }
  }

  return false;
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
    DynamicJsonDocument data(3048);

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
    serializeJson(data, jsonToSend, len + 1);
    webSocket.sendTXT(num, jsonToSend, strlen(jsonToSend));

  }
  break;
  case WStype_TEXT:
  {
    USE_SERIAL.printf("[%u] From Vue: %s\n", num, payload);

    const uint8_t size = JSON_OBJECT_SIZE(5);
    StaticJsonDocument<3048> json;
    DeserializationError err = deserializeJson(json, payload);

    if (err) {
        USE_SERIAL.printf("deserializeJson() failed with code ");
        USE_SERIAL.printf(err.c_str());
        return;
    }

    // serializeJson(json, Serial);

    write(json);
    delay(1000);
    setLEDStripProperties();
    


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
  File file = SPIFFS.open(filename);

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
 * The function parses a color string in hexadecimal format and returns a CRGB object with the
 * corresponding RGB values.
 * 
 * @param colorString A string representing a color in hexadecimal format, starting with the '#'
 * character. For example, "#FF0000" represents the color red.
 * 
 * @return a CRGB object, which represents a color in the RGB color space.
 */
CRGB parseColorString(String colorString) {
  colorString.remove(0, 1);  // Remove the '#' character

  // Extract individual color components
  int r = strtol(colorString.substring(0, 2).c_str(), NULL, 16);
  int g = strtol(colorString.substring(2, 4).c_str(), NULL, 16);
  int b = strtol(colorString.substring(4, 6).c_str(), NULL, 16);

  return CRGB(r, g, b);
}


/**
 * This function reads a file and converts its contents into an Arduino JSON object.
 * 
 * @param filename a string variable that represents the name of the file to be read and converted to
 * Arduino JSON.
 * 
 * @return The function `readFileAndConvertoArduinoJson` returns a `DynamicJsonDocument` object that
 * contains the data read from the file specified by the `filename` parameter, after it has been
 * deserialized from JSON format using the `deserializeJson` function. If there is an error during
 * deserialization, the function returns an empty `DynamicJsonDocument` object.
 */
DynamicJsonDocument readFileAndConvertoArduinoJson(String filename){
  String dataFile = read(filename);

  DynamicJsonDocument json(2048);
  DeserializationError err = deserializeJson(json, dataFile);
  if (err) {
    Serial.println("Error");
    return DynamicJsonDocument(0);
  }

  return json;

}

/**
 * The function dumps a JSON array by iterating through its elements and serializing each element's
 * JSON object.
 * 
 * @param jsonArray A reference to a JSON array object that contains JSON objects as its elements.
 */
void dumpJsonArray(const JsonArray& jsonArray){
  for (size_t i = 0; i < jsonArray.size(); i++){
    JsonObject jsonObject = jsonArray[i].as<JsonObject>();
       serializeJson(jsonObject, Serial);
       USE_SERIAL.println();
  }
}


/**
 * This function reads LED strip properties from a JSON file and applies them to the corresponding LED
 * strips.
 * 
 * @return The function does not have a return type specified, so it does not explicitly return
 * anything.
 */
void setLEDStripProperties(){

  DynamicJsonDocument json(3048);

  String dataFile = read("/data.txt");
  USE_SERIAL.println("[.] Setting LED Strip Properties");
  USE_SERIAL.println(dataFile);

  DeserializationError error = deserializeJson(json, dataFile);



  if (error) {
    Serial.print("Deserialization error: ");
    Serial.println(error.c_str());
    return;
  }

  const JsonArray& shelfs = json["shelfs"].as<JsonArray>();

  int amountOfShelfs = json["config"]["shelfs"].as<int>();
  int pixels = json["config"]["pixels"].as<int>();

  Serial.println("\n[*] APPLYING...");

  Serial.print("[*] Amount of shelfs: ");
  Serial.println(amountOfShelfs);

  Serial.print("[*] Pixels: ");
  Serial.println(pixels);
  
  Serial.println("\n[*] Shelfs:");

  for (const auto& shelf : shelfs){

    int shelfIndex = shelf["shelfIndex"].as<int>();
    clearLEDStripToApplyConfig(shelfIndex);

    int segmentsNumber = shelf["segmentsNumber"].as<int>();

    USE_SERIAL.print("[*] Shelf: ");
    USE_SERIAL.print(shelfIndex + 1);
    USE_SERIAL.print(" [*] Segments: ");
    USE_SERIAL.println(segmentsNumber);

    int iIndex = 0;


    for (int i = 0; i < segmentsNumber; i++){

      String segmentColor = shelf["segments"][i]["color"].as<String>();

      USE_SERIAL.print(" [*] Color: ");
      USE_SERIAL.print(segmentColor);

      // int segmentSize = i == shelf["segments"].size() - 1 ? pixels - shelf["segments"][i - 1]["size"].as<int>() : json["segments"][i]["size"];
      // int segmentSize = shelf["segments"][i]["size"].as<int>();
      int segmentSize;

      // if segment number is 1, then segment size is equal to pixels value , else , to ...

      if (i == segmentsNumber - 1 && segmentsNumber > 1 ){
        // segmentSize = pixels - shelf["segments"][i - 1]["size"].as<int>(); // FIX last segment size
        segmentSize = pixels;
      } else {
        segmentSize = shelf["segments"][i]["size"].as<int>();
      }

      USE_SERIAL.print(" [*] Segment size: ");
      USE_SERIAL.println(segmentSize);

      for(int p = iIndex; p < segmentSize; p++){ // fix this loop

        // USE_SERIAL.print(" [*] p: ");
        // USE_SERIAL.print(p);

        switch (shelfIndex + 1)
        {
        case 1:
          leds1[p] = parseColorString(segmentColor);
          break;
        

        case 2:
          leds2[p] = parseColorString(segmentColor);
          break;
        
        case 3:
          leds3[p] = parseColorString(segmentColor);
          break;
        
        case 4:
          leds4[p] = parseColorString(segmentColor);
          break;
        
        case 5:
          leds5[p] = parseColorString(segmentColor);
          break;
        
        case 6:
          leds6[p] = parseColorString(segmentColor);
          break;

        case 8:
          leds7[p] = parseColorString(segmentColor);
          break;
        
        case 7:
          leds8[p] = parseColorString(segmentColor);
          break;

        default:
          break;
        }
      }


       iIndex = segmentSize;

      // USE_SERIAL.print(" [*] Segment size: ");
      // USE_SERIAL.println(segmentSize);

      // USE_SERIAL.print(" [*] iIndex:  ");
      // USE_SERIAL.println(iIndex);

    }



  }

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
void write(const ArduinoJson::JsonVariantConst& json){
  Serial.println("\n[*] Saving");
  serializeJson(json, Serial);


  Serial.println("\n[*] Opening File");

  File file = SPIFFS.open("/data.txt", "w");

  if(!file){
    Serial.println("[ERROR] There was an error opening the file for writing");
    return;
  }
  file.close();

  file = SPIFFS.open("/data.txt", "w");

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


/**
 * This function clears the LED strip of a specific shelf to apply a new configuration.
 * 
 * @param shelfid The shelf ID is an integer value that represents the specific shelf for which the LED
 * strip needs to be cleared. The function uses a switch statement to determine which LED strip to
 * clear based on the shelf ID provided.
 */
 void clearLEDStripToApplyConfig(int shelfid){

    switch (shelfid)
    {
    case 0:
      fill_solid(leds1, NUM_LEDS, CRGB::Black);
    break;
    case 1:
      fill_solid(leds2, NUM_LEDS, CRGB::Black);
    break;
    case 2:
      fill_solid(leds3, NUM_LEDS, CRGB::Black);
    break;
    case 3:
      fill_solid(leds4, NUM_LEDS, CRGB::Black);
    break;
    case 4:
      fill_solid(leds5, NUM_LEDS, CRGB::Black);
    break;
    case 5:
      fill_solid(leds6, NUM_LEDS, CRGB::Black);
    break;
    case 6:
      fill_solid(leds7, NUM_LEDS, CRGB::Black);
    break;
    case 7:
      fill_solid(leds8, NUM_LEDS, CRGB::Black);
    break;

    default:
      break;
    }

}