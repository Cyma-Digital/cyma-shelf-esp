#include <Arduino.h>
#include "FastLED.h"

#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Ethernet.h>
#include <ETH.h>

#include <WebSocketsServer.h>

#include <SPIFFS.h>
#include <FFat.h>
#include <ArduinoJson.h>

#define USE_SERIAL Serial

FASTLED_USING_NAMESPACE

void stateMachine();
void handleRoot();
void changeColorCategory();
void fadeIn(const CRGB &shelf, int brightness);
void fadeOut(const CRGB &shelf, int brightness);
CRGB hsvToRgb(const CHSV &hsv, CRGB &rgb);
void applyColorCategory();
void applyMidiaColor();
void waitColor();
void handleColor();
void handleProducts();
void handleInteract();
void handleUpdateConfig();
void handleTests();
JsonArray getProducts();
void handleConfigMode();
bool compareColor(CRGB led, CRGB categoryColor);
bool compareColorMidiaProducts(CRGB led, CRGB* categoriesColor, size_t size);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void handleNotFound();
void checkAndWriteToFile();
String read(String filename);
void write(const ArduinoJson::JsonVariantConst &json);
void setLEDStripProperties();
void clearLEDStripToApplyConfig(int shelfid);
CRGB parseColorString(String colorString);
DynamicJsonDocument readFileAndConvertoArduinoJson(String filename);
void dumpJsonArray(const JsonArray &jsonArray);

void printRGB(CRGB rgb);

const int DEFAULT_STATE = 1;
const int POST_REQUEST_STATE = 12;
const int SET_COLOR_STATE = 2;
const int BACK_DEFAULT_STATE = 21;
const int POST_MIDIA_REQUEST_STATE = 3;
int currentState;
CRGB categoryColor;
CRGB midiaColor;

const String responseRedColor = "#ef3e2e";
const String responseYellowColor = "#fdda65";
const String responsePurpleColor = "#6e3896";
const String responseOrangeColor = "#f5833d";
const String responseGreenColor = "#a1cd3a";
const String responseBlueColor = "#47559e";

const String redColor_LED = "#ff0000";
const String yellowColor_LED = "#ff4600";
const String purpleColor_LED = "#9932cc";
const String orangeColor_LED = "#ff1a00";
const String greenColor_LED = "#ddff00";
const String blueColor_LED = "#0000ff";

const String responseRedColor_UPPER = "#EF3E2E";
const String responseYellowColor_UPPER = "#FDDA65";
const String responsePurpleColor_UPPER = "#6E3896";
const String responseOrangeColor_UPPER = "#F5833D";
const String responseGreenColor_UPPER = "#A1CD3A";
const String responseBlueColor_UPPER = "#47559E";

const String redColor_LED_UPPER = "#FF0000";
const String yellowColor_LED_UPPER = "#FF4600";
const String purpleColor_LED_UPPER = "#9932CC";
const String orangeColor_LED_UPPER = "#FF1A00";
const String greenColor_LED_UPPER = "#DDFF00";
const String blueColor_LED_UPPER = "#0000FF";


int delayCategoryColor;

int lastColor = 0;

// esp@192.168.0.10
IPAddress device_IP(192, 168, 0, 10);

// esp@192.168.0.20
// IPAddress device_IP(192,168, 0, 20);

// esp@192.168.0.30
// IPAddress device_IP(192, 168, 0, 30);

// esp@192.168.0.40
// IPAddress device_IP(192,168, 0, 40);

// esp@192.168.0.50
// IPAddress device_IP(192,168, 0, 50);

IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 225, 255, 0);
byte mac[] = {
    0x75, 0xD9, 0xAA, 0x39, 0x90, 0x37};

static bool eth_connected = false;

const char *SSID = "StMarche";
const char *password = NULL;

// const char *jsonString = "{\"config\":{\"shelfs\":1,\"pixels\":40,\"brightness\":127,\"id\":null,\"delay\":10000,\"colors\":[{\"name\":\"Branco\",\"value\":\"#ffffff\"},{\"name\":\"Desligado\",\"value\":\"#000000\"},{\"name\":\"Cinza\",\"value\":\"#808080\"},{\"name\":\"Vermelho\",\"value\":\"#ff0000\"},{\"name\":\"Verde\",\"value\":\"#00ff00\"},{\"name\":\"Azul\",\"value\":\"#0000ff\"},{\"name\":\"Amarelo\",\"value\":\"#ffff00\"},{\"name\":\"Laranja\",\"value\":\"#ffa500\"},{\"name\":\"Rosa\",\"value\":\"#ffc0cb\"},{\"name\":\"Roxo\",\"value\":\"#800080\"},{\"name\":\"Azul Claro\",\"value\":\"#0779bf\"}]},\"shelfs\":[{\"shelfIndex\":0,\"segmentsNumber\":1,\"segments\":[]}]}";
const char *jsonString = "{\"config\":{\"shelfs\":1,\"pixels\":40,\"brightness\":127,\"id\":null,\"delay\":10000,\"products\":[{\"name\":\"SPATEN\",\"id\":1,\"value\":\"#00ff00\"},{\"name\":\"BRAHMACHOPP\",\"id\":2,\"value\":\"#ff0000\"},{\"name\":\"CORONAEXTRA\",\"id\":3,\"value\":\"#ffff00\"},{\"name\":\"BECKS\",\"id\":4,\"value\":\"#00ff00\"},{\"name\":\"BUDWEISER\",\"id\":5,\"value\":\"#ff0000\"},{\"name\":\"STELLAARTOIS\",\"id\":6,\"value\":\"#ffffff\"}]},\"shelfs\":[{\"shelfIndex\":0,\"segmentsNumber\":2,\"segments\":[{\"segmentIndex\":0,\"size\":15,\"product\":1,\"color\":\"#00ff00\"},{\"segmentIndex\":1,\"product\":2,\"color\":\"#ff0000\"}]}]}";
// String* productsColors = nullptr;
CRGB* productsColors = nullptr;

size_t productsColorsSize = 0;

bool configModeActivate = false;
unsigned long interactDelay = 60000;
unsigned long tsconfig; // timestamp config

#define LED_PIN_1 2
// #define LED_PIN_1 13
#define LED_PIN_2 4
#define LED_PIN_3 12
#define LED_PIN_4 14
#define LED_PIN_5 15
#define LED_PIN_6 17
#define LED_PIN_7 5
#define LED_PIN_8 33

#define ETHERNET_CS 33
#define ETH_ADDR 31
#define ETH_POWER_PIN 17
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_TYPE ETH_PHY_TLK110

// #define CONNECTION_MODE "Ethernet"
// #define CONNECTION_MODE "WiFi"
#define CONNECTION_MODE "noWiFi"

WiFiClient client;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(9090);

#define NUM_LEDS 50
// #define NUM_LEDS 10
#define AMOUNT_SHELFS 8
// #define AMOUNT_SHELFS 1

CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGB leds3[NUM_LEDS];
CRGB leds4[NUM_LEDS];
CRGB leds5[NUM_LEDS];
CRGB leds6[NUM_LEDS];
CRGB leds7[NUM_LEDS];
CRGB leds8[NUM_LEDS];

CRGB ref1[NUM_LEDS];
CRGB ref2[NUM_LEDS];
CRGB ref3[NUM_LEDS];
CRGB ref4[NUM_LEDS];
CRGB ref5[NUM_LEDS];
CRGB ref6[NUM_LEDS];
CRGB ref7[NUM_LEDS];
CRGB ref8[NUM_LEDS];

CRGB *leds[AMOUNT_SHELFS] = {leds1, leds2, leds3, leds4, leds5, leds6, leds7, leds8};
CRGB *refs[AMOUNT_SHELFS] = {ref1, ref2, ref3, ref4, ref5, ref6, ref7, ref8};

void setup()
{
  Serial.begin(115200);

  pinMode(5, OUTPUT);

  FastLED.addLeds<WS2812, LED_PIN_1, RGB>(leds[0], 0, NUM_LEDS); // define
  // FastLED.addLeds<WS2812, LED_PIN_1, GRB>(leds[0], 0, NUM_LEDS); // define
  FastLED.addLeds<WS2812, LED_PIN_2, RGB>(leds[1], 0, NUM_LEDS); // define
  FastLED.addLeds<WS2812, LED_PIN_3, RGB>(leds[2], 0, NUM_LEDS); // define
  FastLED.addLeds<WS2812, LED_PIN_4, RGB>(leds[3], 0, NUM_LEDS); // define
  FastLED.addLeds<WS2812, LED_PIN_5, RGB>(leds[4], 0, NUM_LEDS); // define
  FastLED.addLeds<WS2812, LED_PIN_6, RGB>(leds[5], 0, NUM_LEDS); // define
  FastLED.addLeds<WS2812, LED_PIN_7, RGB>(leds[6], 0, NUM_LEDS); // define
  FastLED.addLeds<WS2812, LED_PIN_8, RGB>(leds[7], 0, NUM_LEDS); // define

  fill_solid(leds[0], NUM_LEDS, CRGB::Black);
  fill_solid(leds[1], NUM_LEDS, CRGB::Black);
  fill_solid(leds[2], NUM_LEDS, CRGB::Black);
  fill_solid(leds[3], NUM_LEDS, CRGB::Black);
  fill_solid(leds[4], NUM_LEDS, CRGB::Black);
  fill_solid(leds[5], NUM_LEDS, CRGB::Red);
  fill_solid(leds[6], NUM_LEDS, CRGB::Red);
  fill_solid(leds[7], NUM_LEDS, CRGB::Red);

  FastLED.setBrightness(255);

  // if (!FFat.begin(true))
  // {
  //   Serial.println("Couldn't mount File System");
  // }

  WiFi.config(device_IP, gateway, subnet);

  if (CONNECTION_MODE == "WiFi")
  {
    Serial.println("\n[+] Creating Access Point...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(SSID, password);

    Serial.print("[+] AP Created with IP Gateway ");
    Serial.println(WiFi.softAPIP());
  }
  else
  {
    Serial.println("\n[+] Connecting on WiFi network...");
    // WiFi.begin("CymaDigital", "cyma102030");
    WiFi.begin("DisplayHNK-Net", "cyma102030");
    while (WiFi.status() != WL_CONNECTED)
    {
      USE_SERIAL.print(".");
      delay(100);
    }

    WiFi.mode(WIFI_STA);

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
  }

  server.begin();
  server.on("/", handleRoot);
  server.on("/config-update", handleUpdateConfig);
  server.on("/category", handleColor);
  server.on("/products", handleProducts);
  server.on("/config-mode", handleConfigMode);
  server.on("/interact", handleInteract);
  server.on("/function-test", handleTests);
  server.onNotFound(handleNotFound);


  Serial.println("[+] Creating websocket server... ");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  checkAndWriteToFile();

  currentState = BACK_DEFAULT_STATE;
  USE_SERIAL.print("Current State: ");
  USE_SERIAL.println(currentState);
  // setLEDStripProperties();
}

void loop()
{

  server.handleClient();

  webSocket.loop();

  stateMachine();

  FastLED.show();
}

void stateMachine()
{
  switch (currentState)
  {
  case 1:
    break;
  case 12:
    applyColorCategory();
    lastColor = millis();
    currentState = 2;
    break;
  case 2:
    waitColor();
    break;
  case 21:
    setLEDStripProperties();
    currentState = DEFAULT_STATE;
    break;
  case 3:
    applyMidiaColor();
    lastColor = millis();
    currentState = 2;
  default:
    break;
  }
}

void waitColor()
{

  if (lastColor + delayCategoryColor < millis())
  {
    currentState = BACK_DEFAULT_STATE;
    USE_SERIAL.print("Current State: ");
    USE_SERIAL.println(currentState);
  }
}

void handleRoot()
{

  Serial.println("Handle Root");

  SPIFFS.begin();
  File file = SPIFFS.open("/index.html.gz", "r");
  server.streamFile(file, "text/html");
  file.close();
  SPIFFS.end();
}

void changeColorCategory()
{
  lastColor = millis();
  currentState = SET_COLOR_STATE;
  USE_SERIAL.print("Current State: ");
  USE_SERIAL.println(currentState);
  applyColorCategory();
}

void fadeOut(CRGB &shelf, int brightness)
{
  shelf.maximizeBrightness(255 - brightness);
}

void fadeIn(CRGB &shelf, int brightness)
{
  shelf.maximizeBrightness(0 + brightness);
}

void applyColorCategory()
{

  for (int i = 0; i <= 255; i += 5)
  {
    for (int j = 0; j < NUM_LEDS; j++)
    {

      for (int k = 0; k < AMOUNT_SHELFS; k++)
      {
        if (compareColor(refs[k][j], categoryColor))
        {
          // cor diferente
          // leds[k][j] --;
          fadeOut(leds[k][j], i);
        }
        else
        {
          // cor igual
          leds[k][j].r += leds[k][j].r < refs[k][j].r ? 5 : 0;
          leds[k][j].g += leds[k][j].g < refs[k][j].g ? 5 : 0;
          leds[k][j].b += leds[k][j].b < refs[k][j].b ? 5 : 0;
        }
      }
    }
    FastLED.show();
  }
  USE_SERIAL.print("Current State: ");
  USE_SERIAL.println(currentState);
  return;
}


void applyMidiaColor(){

  for (size_t i = 0; i <= 255; i++)
  {
    for(size_t j = 0; j < NUM_LEDS; j++) 
    {
      for (size_t k = 0; k < 1; k++) {
        if(compareColorMidiaProducts(refs[k][j], productsColors, productsColorsSize))
        {

          leds[k][j].r += leds[k][j].r < midiaColor.r ? 5 : 0;
          leds[k][j].g += leds[k][j].g < midiaColor.g ? 5 : 0;
          leds[k][j].b += leds[k][j].b < midiaColor.b ? 5 : 0;

        } else {
          fadeOut(leds[k][j], i);
        }
      }
    }
    FastLED.show();
  }
  
}

void handleColor()
{

    Serial.println("\nHandler Category");
    USE_SERIAL.print("Current State: ");
    USE_SERIAL.println(currentState);
    String response;

    if(configModeActivate == true && tsconfig + interactDelay < millis()){
      Serial.println("Config mode is false");
      configModeActivate = false;
      response = "{\"message\":\"config mode false\"}";
      server.send(200, "application/json", response);
      return;
    }


    if(configModeActivate == false){

      Serial.println("\nHandler Category");

     if (server.hasArg("plain") == false)
     {
       return;
     }
  
     DynamicJsonDocument json(2048);
     String body = server.arg("plain");
     DeserializationError error = deserializeJson(json, body);
     if (error)
     {
       Serial.println("Error");
       return;
     }
  
     serializeJson(json, Serial);
     USE_SERIAL.println();
  
     String colorFromTerminal = json["color"].as<String>();
     categoryColor = parseColorString(colorFromTerminal);
  
     currentState = POST_REQUEST_STATE;
  
     response = "{\"message\":\"success\"}";
     server.send(200, "application/json", response);
  
    }

    response = "{\"message\":\"config mode is true\"}";
    server.send(200, "application/json", response);
}

void handleProducts(){

  Serial.println("Handle Products");

  String response;

  if (configModeActivate == true && tsconfig + interactDelay < millis()) {
    Serial.println("Config mode is false");
    configModeActivate = false;
    server.send(200, "application/json", response);
    return;
  }

  if(configModeActivate == false) {

    String request_body;

    DynamicJsonDocument tempJson(2048);


    if(server.hasArg("plain") == false) {
      response = "{\"message\":\"empty body\"}";
      server.send(200, "application/json", response);
      return;
    }

    request_body = server.arg("plain");


    DeserializationError error = deserializeJson(tempJson, request_body);
    if(error) {
      Serial.println("Erro");
      return;
    }

    serializeJson(tempJson, Serial);
    Serial.println();

    String color = tempJson["cor"].as<String>();


    if(tempJson.containsKey("produtos") && tempJson["produtos"].is<JsonArray>()) {
      JsonArray produtosArray = tempJson["produtos"].as<JsonArray>();

      
      productsColors = new CRGB[produtosArray.size()];
      productsColorsSize = produtosArray.size();

      JsonArray storegeProducts = getProducts();


      for(size_t i = 0; i < produtosArray.size(); i++) {

        for(size_t j = 0; j < storegeProducts.size(); j++) {
          if(produtosArray[i].as<int>() == storegeProducts[j]["id"].as<int>()){
            // Serial.println(storegeProducts[j]["name"].as<String>());
            // Serial.println(storegeProducts[j]["id"].as<int>());

            productsColors[i] = parseColorString(storegeProducts[j]["value"].as<String>()); 
            
          }
        }

      }

      
      // for(size_t i = 0; i < produtosArray.size(); i++) {
      //   Serial.println(productsColors[i].r);
      // }

    } 

    midiaColor = parseColorString(color);

    currentState = POST_MIDIA_REQUEST_STATE;

    // Send color to antoher function 
    response = "{\"message\":\"success\"}";
    server.send(200, "application/json", response);
    return;

  }

  response = "{\"message\":\"config mode is true\"}";
  server.send(200, "application/json", response);
  
}

void handleUpdateConfig(){
  Serial.println("HandleUpdateConfig");


  String response;
  String request_body;
  DynamicJsonDocument tempJson(4048);

  if(server.hasArg("plain") == false) {
    response = "{\"message\":\"Empty body\"}";
    server.send(200, "application/json", response);
    return;
  }


  request_body = server.arg("plain");


  DeserializationError error = deserializeJson(tempJson, request_body);
  if(error){
    Serial.print("Error: ");
    Serial.println(error.c_str());

    response = "{\"message\":\"Deserialization Error\"}";
    server.send(500, "application/json", response);
  }

  write(tempJson);
  
  setLEDStripProperties();
  response = "{\"message\":\"Configuration updated\"}";
  server.send(200, "application/json", response);


}

// This funcion handler must be used to test some function without wait to all the code flow
void handleTests(){
  Serial.println("Handle Tests");

  String response;
  String request_body;

  JsonArray products;

  products = getProducts();

  for(size_t i = 0; i < products.size(); i++) {
    Serial.println(products[i]["name"].as<String>());
    Serial.println(products[i]["id"].as<int>());
  }


  // if(server.hasArg("plain") == false) {
  //   response = "{\"message\":\"empty body\"}";
  //   server.send(200, "application/json", response);
  //   return;
  // }

  // request_body = server.arg("plain");


  // DeserializationError error = deserializeJson(tempJson, request_body);
  // if(error) {
  //   Serial.println("Erro");
  //   return;
  // }

  // serializeJson(tempJson, Serial);
  // Serial.println();

  response = "{\"message\":\"success\"}";
  server.send(200, "application/json", response);
  return;

  
}

JsonArray getProducts() {

  DynamicJsonDocument json = readFileAndConvertoArduinoJson("/data.txt");
  JsonArray products;

  if(json.containsKey("config") && json["config"].containsKey("products") && json["config"]["products"].is<JsonArray>()){
    products = json["config"]["products"].as<JsonArray>();
    

  }
  
  return products;

}


void handleConfigMode(){
  Serial.println("Handle Configuration Mode");

  String response;
  String request_body;

  DynamicJsonDocument tempJson(1024);

  if(server.hasArg("plain") == false) {
    response = "{\"message\":\"empty body\"}";
    server.send(200, "application/json", response);
    return;
  }

  request_body = server.arg("plain");

  DeserializationError error = deserializeJson(tempJson, request_body);
  if(error) {
    Serial.println("Erro");
    return;
  }


  serializeJson(tempJson, Serial);

  configModeActivate = tempJson["status"].as<boolean>();
  Serial.println(configModeActivate);

  currentState = BACK_DEFAULT_STATE;
  
  response = "{\"message\":\"success\"}";
  server.send(200, "application/json", response);
  return;

}


void handleInteract(){
  Serial.println("Handle Interact");

  tsconfig = millis();


  String response;
  response = "{\"message\":\""+ String(tsconfig)+"\"}";
  server.send(200, "application/json", response);
  return;
}

bool compareColor(CRGB led, CRGB categoriesColor)
{

  // USE_SERIAL.println(String("FadeLightBy") + " (" + String(led.r) + "," + String(led.g) + "," + String(led.b) + ")");
  if (led.r != categoriesColor.r || led.g != categoriesColor.g || led.b != categoriesColor.b)
  {
    return true;
  }

  return false;
  
}

bool compareColorMidiaProducts(CRGB led, CRGB* categoriesColor, size_t size)
{

  for( int x=0; x < size; x++)
  {

    if (led.r == categoriesColor[x].r && led.g == categoriesColor[x].g && led.b == categoriesColor[x].b)
    {
      return true;
    }

  }

  return false;

}


void handleNotFound()
{
  Serial.println("Handle not Found");
  server.send(404, "text/plain", "404: Not found");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

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
    DynamicJsonDocument data(4048);

    String dataFile = read("/data.txt");

    DeserializationError error = deserializeJson(data, dataFile);

    // Check for any errors during deserialization
    if (error)
    {
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

    if (err)
    {
      USE_SERIAL.printf("deserializeJson() failed with code ");
      USE_SERIAL.printf(err.c_str());
      return;
    }

    write(json);
    // delay(1000);
    setLEDStripProperties();
  }
  break;
  default:
    break;
  }
}


void printRGB(CRGB rgb){
  Serial.print("(");
  Serial.print(rgb.r);
  Serial.print(",");
  Serial.print(rgb.g);
  Serial.print(",");
  Serial.print(rgb.b);
  Serial.print(") ");
}

/**
 * The function checks if a file exists and contains data, and if not, writes default data to it.
 *
 * @return nothing (void).
 */
void checkAndWriteToFile()
{

  String fileContent;

  SPIFFS.begin();
  File file = SPIFFS.open("/data.txt", "r");
  if (!file)
  {
    Serial.println("There was an error opening the file for writing");
    return;
  }

  int fileSize = file.size();
  Serial.print("[*] File size: ");
  Serial.print(fileSize);
  Serial.println(" bytes");

  if (file.size() == 0)
  {

    File file = SPIFFS.open("/data.txt", "w");
    Serial.println("[!] Blank file");
    if (file.print(jsonString))
    {
      Serial.println("[+] Default data written");
      file.close();

      SPIFFS.begin();
      File fileData = SPIFFS.open("/data.txt", "r");
      int len = fileData.size();

      char buffer[len];

      if (!fileData)
      {
        Serial.println("[ERROR] Failed to open file for reading");
        return;
      }

      while (fileData.available())
      {
        fileContent += (char)fileData.read();
      }

      Serial.println("\n[+] File Content:");
      Serial.println(fileContent);
      Serial.println();

      fileData.close();
      return;
    }
    else
    {
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
String read(String filename)
{
  String fileContent;

  SPIFFS.begin();
  File file = SPIFFS.open(filename);

  if (!file)
  {
    Serial.println("[ERROR] Failed to open file for reading");
    return "None";
  }

  while (file.available())
  {
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
CRGB parseColorString(String colorString)
{

  // Serial.println(colorString);
  // if (colorString == responseRedColor_UPPER)
  //   colorString = redColor_LED_UPPER;
  // else if (colorString == responseYellowColor_UPPER)
  //   colorString = yellowColor_LED_UPPER;
  // else if (colorString == responsePurpleColor_UPPER)
  //   colorString = purpleColor_LED_UPPER;
  // else if (colorString == responseOrangeColor_UPPER)
  //   colorString = orangeColor_LED_UPPER;
  // else if (colorString == responseGreenColor_UPPER)
  //   colorString = greenColor_LED_UPPER;
  // else if (colorString == responseBlueColor_UPPER)
  //   colorString = blueColor_LED_UPPER;
  // else
  //   Serial.println("Cor inválida");

  // Map<String, String> colorMapping = new HashMap<>();
  // colorMapping.put(responseRedColor_UPPER, redColor_LED_UPPER);
  // colorMapping.put(responseYellowColor_UPPER, yellowColor_LED_UPPER);
  // colorMapping.put(responsePurpleColor_UPPER, purpleColor_LED_UPPER);
  // colorMapping.put(responsePinkColor_UPPER, pinkColor_LED_UPPER);
  // colorMapping.put(responseGreenColor_UPPER, greenColor_LED_UPPER);
  // colorMapping.put(responseBlueColor_UPPER, blueColor_LED_UPPER);

  // if (colorMapping.containsKey(colorString))
  // {
  //   colorString = colorMapping.get(colorString);
  // }

  colorString.remove(0, 1); // Remove the '#' character

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
DynamicJsonDocument readFileAndConvertoArduinoJson(String filename)
{
  String dataFile = read(filename);

  DynamicJsonDocument json(2048);
  DeserializationError err = deserializeJson(json, dataFile);
  if (err)
  {
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
void dumpJsonArray(const JsonArray &jsonArray)
{
  for (size_t i = 0; i < jsonArray.size(); i++)
  {
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
void setLEDStripProperties()
{

  DynamicJsonDocument json(4048);

  String dataFile = read("/data.txt");

  DeserializationError error = deserializeJson(json, dataFile);

  if (error)
  {
    Serial.print("Deserialization error: ");
    Serial.println(error.c_str());
    return;
  }

  const JsonArray &shelfs = json["shelfs"].as<JsonArray>();

  int amountOfShelfs = json["config"]["shelfs"].as<int>();
  int pixels = json["config"]["pixels"].as<int>();
  int brightness = json["config"]["brightness"].as<int>();
  int delayColor = json["config"]["delay"].as<int>();

  delayCategoryColor = delayColor;

  FastLED.setBrightness(brightness);
  // Serial.println("\n[*] APPLYING...");

  // Serial.print("[*] Amount of shelfs: ");
  // Serial.println(amountOfShelfs);

  // Serial.print("[*] Pixels: ");
  // Serial.println(pixels);

  // Serial.println("\n[*] Shelfs:");

  for (const auto &shelf : shelfs)
  {

    int shelfIndex = shelf["shelfIndex"].as<int>();
    clearLEDStripToApplyConfig(shelfIndex);

    int segmentsNumber = shelf["segmentsNumber"].as<int>();

    // USE_SERIAL.print("[*] Shelf: ");
    // USE_SERIAL.print(shelfIndex + 1);
    // USE_SERIAL.print(" [*] Segments: ");
    // USE_SERIAL.println(segmentsNumber);

    int iIndex = 0;

    for (int i = 0; i < segmentsNumber; i++)
    {

      String segmentColor = shelf["segments"][i]["color"].as<String>();

      // if (segmentColor == responseRedColor)
      //   segmentColor = redColor_LED;
      // else if (segmentColor == responseYellowColor)
      //   segmentColor = yellowColor_LED;
      // else if (segmentColor == responsePurpleColor)
      //   segmentColor = purpleColor_LED;
      // else if (segmentColor == responseOrangeColor)
      //   segmentColor = orangeColor_LED;
      // else if (segmentColor == responseGreenColor)
      //   segmentColor = greenColor_LED;
      // else if (segmentColor == responseBlueColor)
      //   segmentColor = blueColor_LED;
      // else
      //   Serial.println("Cor inválida");

      // Map<String, String> colorMapping = new HashMap<>();
      // colorMapping.put(responseRedColor, redColor_LED);
      // colorMapping.put(responseYellowColor, yellowColor_LED);
      // colorMapping.put(responsePurpleColor, purpleColor_LED);
      // colorMapping.put(responsePinkColor, pinkColor_LED);
      // colorMapping.put(responseGreenColor, greenColor_LED);
      // colorMapping.put(responseBlueColor, blueColor_LED);

      // if (colorMapping.containsKey(segmentColor))
      // {
      //   segmentColor = colorMapping.get(segmentColor);
      // }

      int segmentSize;

      // if segment number is 1, then segment size is equal to pixels value , else , to ...

      if (i == segmentsNumber - 1 && segmentsNumber > 1)
      {
        // segmentSize = pixels - shelf["segments"][i - 1]["size"].as<int>(); // FIX last segment size
        segmentSize = pixels;
      }
      else
      {
        segmentSize = shelf["segments"][i]["size"].as<int>();
      }

      // USE_SERIAL.print(" [*] Segment size: ");
      // USE_SERIAL.println(segmentSize);

      for (int p = iIndex; p < segmentSize; p++)
      {

        for (int j = 0; j < AMOUNT_SHELFS; j++)
        {
          leds[shelfIndex][p] = parseColorString(segmentColor);
          refs[shelfIndex][p] = parseColorString(segmentColor);
        }

        iIndex = segmentSize;
      }
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
void write(const ArduinoJson::JsonVariantConst &json)
{
  Serial.println("\n[*] Saving");
  serializeJson(json, Serial);

  Serial.println("\n[*] Opening File");

  File file = SPIFFS.open("/data.txt", "w");

  if (!file)
  {
    Serial.println("[ERROR] There was an error opening the file for writing");
    return;
  }
  file.close();

  file = SPIFFS.open("/data.txt", "w");

  if (!file)
  {
    Serial.println("There was an error reopening the file for writing");
    return;
  }

  if (serializeJson(json, file) == 0)
  {
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
void clearLEDStripToApplyConfig(int shelfid)
{
  fill_solid(leds[shelfid], NUM_LEDS, CRGB::Black);
}

/* !! Uncomment the functions below the Ethernet module being used in the ESP32 */

/* ENC28j60 or W5100 */

/**
 * The function initializes the Ethernet shield, checks for hardware presence, and prints the status of
 * the Ethernet connection.
 */
