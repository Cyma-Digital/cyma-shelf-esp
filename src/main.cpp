#include <Arduino.h>
#include "FastLED.h"

#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Ethernet.h>
#include <ETH.h>
#include <vector>
#include <sstream>
#include <string>
#include <iostream>

#include <WebSocketsServer.h>

#include <SPIFFS.h>
#include <FFat.h>
#include <ArduinoJson.h>

#define USE_SERIAL Serial

FASTLED_USING_NAMESPACE

// LED CONTROLLER
void fadeInAllSegments(CRGB color);
void fadeInAllSegmentsBasedOnCurrentMode();
void clearLEDStripToApplyConfig(int shelfid);
void clearStrip(std::string mode);
void clearAllStrips();
void setLEDStripProperties();
uint8_t getConfigBrightness();

void fadeOut(const CRGB &shelf, int brightness);
void fadeInSegment(CRGB &shelf, CRGB color);
void fadeInSegmentBasedOnCurrentMode(CRGB &shelf, CRGB color);
void fadeOutToIn();

// NETWORK MANAGER
void handleRoot();
void handleConfig();
void handleColor();
void handleProducts();
void handleLightUpAllColors();
void handleNotFound();
void handleConfigMode();
void handleInteract();
void handleTests();

// FILE MANAGER
String read(String filename);
void write(const ArduinoJson::JsonVariantConst &json);
DynamicJsonDocument readFileAndConvertoArduinoJson(String filename);

void checkAndWriteToFile();

// STATE MACHINE
void stateMachine();
const int DEFAULT_STATE = 1;
const int POST_REQUEST_STATE = 12;
const int SET_COLOR_STATE = 2;
const int BACK_DEFAULT_STATE = 21;
const int POST_MIDIA_REQUEST_STATE = 3;

void applyColorCategory();
void applyMidiaColor();
void waitColor();

// CONFIG MANAGER
void loadColorModeConfig(const DynamicJsonDocument &json);
bool shouldDeactivateConfigMode();

std::string colorMode = "cold";
std::string defaultColdColor = "#FFFFFF";
std::string defaultWarmColor = "#965511";

void changeColorCategory();
CRGB hsvToRgb(const CHSV &hsv, CRGB &rgb);

CRGB getDefaultColor();

JsonArray getProducts();
bool compareColor(CRGB led, CRGB categoryColor);
bool compareColorMidiaProducts(CRGB led, CRGB *categoriesColor, size_t size);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
CRGB parseColorString(String colorString);
std::vector<int> parseRGBString(const std::string &rgbString);
void dumpJsonArray(const JsonArray &jsonArray);
int getFadeStep(uint8_t brightness);
void printRGB(CRGB rgb);

int currentState;

/* NETWORK CONFIGURATION */

// esp@192.168.0.10
// IPAddress device_IP(192, 168, 0, 10);

// esp@192.168.0.20
// IPAddress device_IP(192,168, 0, 20);

// esp@192.168.0.30
// IPAddress device_IP(192, 168, 0, 30);

// esp@192.168.0.40
// IPAddress device_IP(192,168, 0, 40);

// esp@192.168.0.50
IPAddress device_IP(192, 168, 20, 50);

// IPAddress gateway(192, 168, 0, 1);
IPAddress gateway(192, 168, 20, 1);
IPAddress subnet(255, 225, 255, 0);
byte mac[] = {
    0x75, 0xD9, 0xAA, 0x39, 0x90, 0x37};

static bool eth_connected = false;

const char *SSID = "StMarche";
const char *password = NULL;

// #define CONNECTION_MODE "Ethernet"
// #define CONNECTION_MODE "WiFi"
#define CONNECTION_MODE "noWiFi"

/* LED CONTROLLER CONFIGURATION */
const char *jsonString = "{\"config\":{\"shelfs\":1,\"pixels\":11,\"title\":\"Gondola Ambev\",\"colorMode\":\"warm\",\"defaultWarmColor\":\"#965511\",\"defaultColdColor\":\"#FFFFFF\",\"pixelCentimeters\":5,\"brightness\":255,\"id\":null,\"delay\":10000,\"interactDelay\":60000,\"products\":[{\"name\":\"SPATEN\",\"id\":1,\"value\":\"#00ff00\"},{\"name\":\"BRAHMACHOPP\",\"id\":2,\"value\":\"#ff0000\"},{\"name\":\"CORONAEXTRA\",\"id\":3,\"value\":\"#ffff00\"},{\"name\":\"BECKS\",\"id\":4,\"value\":\"#00ff00\"},{\"name\":\"BUDWEISER\",\"id\":5,\"value\":\"#ff0000\"},{\"name\":\"STELLAARTOIS\",\"id\":6,\"value\":\"#ffffff\"}]},\"shelfs\":[{\"shelfIndex\":0,\"segmentsNumber\":5,\"segments\":[{\"segmentIndex\":0,\"size\":1,\"realSize\":1,\"product\":6,\"color\":\"#ffffff\"},{\"segmentIndex\":1,\"size\":2,\"realSize\":1,\"product\":5,\"color\":\"#ff0000\"},{\"segmentIndex\":2,\"size\":4,\"realSize\":2,\"product\":1,\"color\":\"#00ff00\"},{\"segmentIndex\":3,\"size\":7,\"realSize\":3,\"product\":3,\"color\":\"#ffff00\"},{\"segmentIndex\":4,\"size\":10,\"realSize\":3,\"product\":2,\"color\":\"#ff0000\"}]}]}";

CRGB categoryColor;
CRGB midiaColor;
int delayCategoryColor;
int lastColor = 0;
CRGB *productsColors = nullptr;
size_t productsColorsSize = 0;
bool configModeActivate = false;
unsigned long interactDelay = 60000;
unsigned long lastInteractionTimestamp;

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

WiFiClient client;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(9090);

#define NUM_LEDS 120
#define AMOUNT_SHELFS 8

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
  fill_solid(leds[5], NUM_LEDS, CRGB::Black);
  fill_solid(leds[6], NUM_LEDS, CRGB::Black);
  fill_solid(leds[7], NUM_LEDS, CRGB::Black);

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
    WiFi.begin("CraftTouch-Control", "xUy!wajcw9");

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
  server.enableCORS();
  server.enableCrossOrigin();
  server.on("/", handleRoot);
  server.on("/config", handleRoot);
  server.on("/config-handle", handleConfig);
  server.on("/category", handleColor);
  server.on("/products", handleProducts);
  server.on("/all", handleLightUpAllColors);
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
  setLEDStripProperties();
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
  case DEFAULT_STATE:
    break;
  case POST_REQUEST_STATE:
    applyColorCategory();
    lastColor = millis();
    currentState = 2;
    break;
  case SET_COLOR_STATE:
    waitColor();
    break;
  case BACK_DEFAULT_STATE:
    clearStrip("exit_config_mode");
    currentState = DEFAULT_STATE;
    break;
  case POST_MIDIA_REQUEST_STATE:
    clearStrip("products_fade");
    lastColor = millis();
    currentState = SET_COLOR_STATE;
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

void fadeInSegment(CRGB &shelf, CRGB color)
{
  shelf.r += shelf.r < color.r ? 5 : 0;
  shelf.g += shelf.g < color.g ? 5 : 0;
  shelf.b += shelf.b < color.b ? 5 : 0;
}

void fadeInSegmentBasedOnCurrentMode(CRGB &shelf, CRGB color)
{
  if (colorMode == "warm")
  {
    fadeInSegment(shelf, parseColorString(defaultWarmColor.c_str()));
  }
  else if (colorMode == "cold")
  {
    fadeInSegment(shelf, parseColorString(defaultColdColor.c_str()));
  }
  else
  {
    fadeInSegment(shelf, color);
  }
}

void fadeInAllSegments(CRGB color)
{
  for (size_t i = 0; i <= 255; i++)
  {
    for (size_t j = 0; j < NUM_LEDS; j++)
    {
      for (size_t k = 0; k < AMOUNT_SHELFS; k++)
      {
        fadeInSegment(leds[k][j], color);
      }
    }
    // FastLED.show();
  }
}

void fadeInAllSegmentsBasedOnCurrentMode()
{
  for (size_t i = 0; i <= 255; i++)
  {
    for (size_t j = 0; j < NUM_LEDS; j++)
    {
      for (size_t k = 0; k < AMOUNT_SHELFS; k++)
      {
        fadeInSegmentBasedOnCurrentMode(leds[k][j], refs[k][j]);
      }
    }
    FastLED.show();
  }
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
          fadeOut(leds[k][j], i);
        }
        else
        {
          fadeInSegmentBasedOnCurrentMode(leds[k][j], refs[k][j]);
        }
      }
    }
    FastLED.show();
  }
  USE_SERIAL.print("Current State: ");
  USE_SERIAL.println(currentState);
  return;
}

void applyMidiaColor()
{

  for (size_t i = 0; i <= 255; i += 5)
  {
    for (size_t j = 0; j < NUM_LEDS; j++)
    {
      for (size_t k = 0; k < AMOUNT_SHELFS; k++)
      {
        if (compareColorMidiaProducts(refs[k][j], productsColors, productsColorsSize))
        {
          fadeInSegmentBasedOnCurrentMode(leds[k][j], midiaColor);
          // fadeInSegment(leds[k][j], midiaColor);
        }
        else
        {
          fadeOut(leds[k][j], i);
        }
      }
    }
    // FastLED.show();
  }
}

bool shouldDeactivateConfigMode()
{
  return configModeActivate == true && lastInteractionTimestamp + interactDelay < millis();
}

void handleColor()
{

  Serial.println("\nHandler Category");
  USE_SERIAL.print("Current State: ");
  USE_SERIAL.println(currentState);
  String response;

  if (shouldDeactivateConfigMode())
  {
    Serial.println("Config mode is false");
    configModeActivate = false;
    response = "{\"message\":\"config mode false\"}";
    server.send(200, "application/json", response);
    return;
  }

  if (configModeActivate == false)
  {

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

/**
 * The function `handleProducts` processes product information and color data, responding with success
 * messages or error messages accordingly.
 *
 * @return The function `handleProducts()` returns a JSON response with a message indicating the status
 * of the operation.
 */
void handleProducts()
{

  Serial.println("Handle Products");

  String response;

  if (shouldDeactivateConfigMode())
  {
    Serial.println("Config mode is false");
    configModeActivate = false;
    server.send(200, "application/json", response);
    return;
  }

  if (configModeActivate == false)
  {

    String request_body;

    DynamicJsonDocument tempJson(4048);

    if (server.hasArg("plain") == false)
    {
      response = "{\"message\":\"empty body\"}";
      server.send(200, "application/json", response);
      return;
    }

    request_body = server.arg("plain");

    DeserializationError error = deserializeJson(tempJson, request_body);
    if (error)
    {
      Serial.print("Error: ");
      Serial.println(error.c_str());
      response = "{\"message\":\"" + String(error.c_str()) + "\"}";
      server.send(500, "application/json", response);
    }

    serializeJson(tempJson, Serial);
    Serial.println();

    String color = tempJson["cor"].as<String>();

    if (tempJson.containsKey("produtos") && tempJson["produtos"].is<JsonArray>())
    {
      JsonArray produtosArray = tempJson["produtos"].as<JsonArray>();

      productsColors = new CRGB[produtosArray.size()];
      productsColorsSize = produtosArray.size();

      JsonArray storegeProducts = getProducts();

      for (size_t i = 0; i < produtosArray.size(); i++)
      {
        for (size_t j = 0; j < storegeProducts.size(); j++)
        {
          if (produtosArray[i].as<int>() == storegeProducts[j]["id"].as<int>())
          {
            productsColors[i] = parseColorString(storegeProducts[j]["value"].as<String>());
          }
        }
      }
    }

    midiaColor = parseColorString(color);

    currentState = POST_MIDIA_REQUEST_STATE;

    response = "{\"message\":\"success\"}";
    server.send(200, "application/json", response);
    return;
  }

  response = "{\"message\":\"config mode is true\"}";
  server.send(200, "application/json", response);
}

void handleLightUpAllColors()

{

  clearStrip("exit_config_mode");
  CRGB defaultColor = getDefaultColor();

  fadeInAllSegments(defaultColor);

  server.send(200, "application/json", "{\"message\":\"success\"}");

  currentState = DEFAULT_STATE;
}

CRGB getDefaultColor()
{
  if (colorMode == "warm")
  {
    return parseColorString(defaultWarmColor.c_str());
  }
  else if (colorMode == "cold")
  {
    return parseColorString(defaultColdColor.c_str());
  }
  else
  {
    return parseColorString(defaultColdColor.c_str());
  }
}

void handleConfig()
{
  Serial.println("Handle Config");

  String response;

  if (server.method() == HTTP_GET)
  {

    SPIFFS.begin();
    File file = SPIFFS.open("/data.txt", "r");
    server.streamFile(file, "application/json");
    file.close();
    SPIFFS.begin();
  }
  else if (server.method() == HTTP_POST)
  {

    String request_body;
    DynamicJsonDocument tempJson(4048);

    if (server.hasArg("plain") == false)
    {
      response = "{\"message\":\"Empty body\"}";
      server.send(200, "application/json", response);
      return;
    }

    request_body = server.arg("plain");

    DeserializationError error = deserializeJson(tempJson, request_body);
    if (error)
    {
      Serial.print("Error: ");
      Serial.println(error.c_str());
      response = "{\"message\":\"" + String(error.c_str()) + "\"}";
      server.send(500, "application/json", response);
    }

    write(tempJson);

    setLEDStripProperties();
    response = "{\"message\":\"Configuration updated\"}";
    server.send(200, "application/json", response);
  }
  else
  {
    response = "{\"message\":\"Method Not Allowed\"}";
    server.send(405, "application/json", response);
  }
}

// This funcion handler must be used to test some function without wait to all the code flow
void handleTests()
{
  Serial.println("Handle Tests");

  String response;
  String request_body;

  // JsonArray products;

  // products = getProducts();

  // for(size_t i = 0; i < products.size(); i++) {
  //   Serial.println(products[i]["name"].as<String>());
  //   Serial.println(products[i]["id"].as<int>());
  // }

  DynamicJsonDocument tempJson(2048);

  if (server.hasArg("plain") == false)
  {
    response = "{\"message\":\"empty body\"}";
    server.send(200, "application/json", response);
    return;
  }

  request_body = server.arg("plain");

  DeserializationError error = deserializeJson(tempJson, request_body);
  if (error)
  {
    Serial.println("Erro");
    return;
  }

  uint8_t currentBrightness = getConfigBrightness();

  int step = getFadeStep(currentBrightness);

  for (int i = currentBrightness; i >= 0; i -= step)
  {

    FastLED.setBrightness(i);
    FastLED.show();
  }

  FastLED.setBrightness(0);
  FastLED.show();

  delay(2000);
  for (int i = 0; i <= currentBrightness; i += step)
  {
    FastLED.setBrightness(i);
    FastLED.show();
  }

  // serializeJson(tempJson, Serial);

  // std::string color = tempJson["color"].as<std::string>();

  // std::vector<int> colorArray = parseRGBString(color);
  // CRGB currentColor(colorArray[0], colorArray[1], colorArray[2]);
  // fill_solid(leds[0], NUM_LEDS, currentColor);
  // fill_solid(leds[1], NUM_LEDS, currentColor);

  Serial.println();

  response = "{\"message\":\"success\"}";
  server.send(200, "application/json", response);
  return;
}

/**
 * The function `getProducts()` reads a JSON file, extracts the "products" array from it, and returns
 * the array.
 *
 * @return a JsonArray object named "products".
 */
JsonArray getProducts()
{

  DynamicJsonDocument json = readFileAndConvertoArduinoJson("/data.txt");
  JsonArray products;

  if (json.containsKey("config") && json["config"].containsKey("products") && json["config"]["products"].is<JsonArray>())
  {
    products = json["config"]["products"].as<JsonArray>();
  }

  return products;
}

void handleConfigMode()
{
  Serial.println("Handle Configuration Mode");

  String response;
  String request_body;

  DynamicJsonDocument tempJson(1024);

  if (server.hasArg("plain") == false)
  {
    response = "{\"message\":\"empty body\"}";
    server.send(200, "application/json", response);
    return;
  }

  request_body = server.arg("plain");

  DeserializationError error = deserializeJson(tempJson, request_body);
  if (error)
  {
    Serial.println("Erro");
    return;
  }

  serializeJson(tempJson, Serial);

  configModeActivate = tempJson["status"].as<boolean>();

  if (!configModeActivate)
  {
    clearStrip("exit_config_mode");
    response = "{\"message\":\"config mode is false\"}";
    server.send(200, "application/json", response);
    return;
  }

  lastInteractionTimestamp = millis();
  clearStrip("config_mode");
  // currentState = BACK_DEFAULT_STATE;
  // setLEDStripProperties();


  response = "{\"message\":\"success\"}";
  server.send(200, "application/json", response);
  return;
}

void handleInteract()
{
  Serial.println("Handle Interact");

  lastInteractionTimestamp = millis();
  configModeActivate = true;

  String response = "{\"message\":\"" + String(lastInteractionTimestamp) + "\"}";

  server.send(200, "application/json", response);
  return;
}

bool compareColor(CRGB led, CRGB categoriesColor)
{

  if (led.r != categoriesColor.r || led.g != categoriesColor.g || led.b != categoriesColor.b)
  {
    return true;
  }

  return false;
}

bool compareColorMidiaProducts(CRGB led, CRGB *categoriesColor, size_t size)
{

  for (int x = 0; x < size; x++)
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

void printRGB(CRGB rgb)
{
  Serial.print("(");
  Serial.print(rgb.r);
  Serial.print(",");
  Serial.print(rgb.g);
  Serial.print(",");
  Serial.print(rgb.b);
  Serial.print(") ");
}

int getFadeStep(uint8_t brightness)
{
  if (brightness >= 204)
    return 17; // Nível mais alto, próximo a 255
  else if (brightness >= 153)
    return 13; // Nível alto
  else if (brightness >= 102)
    return 10; // Nível médio
  else if (brightness >= 51)
    return 7; // Nível baixo
  else
    return 1; // Nível mais baixo
}

uint8_t getConfigBrightness()
{
  DynamicJsonDocument json(4048);

  String dataFile = read("/data.txt");

  DeserializationError error = deserializeJson(json, dataFile);

  if (error)
  {
    Serial.print("Deserialization error: ");
    Serial.println(error.c_str());
    return 255;
  }

  uint8_t brightness = json["config"]["brightness"].as<uint8_t>();

  return brightness;
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
 * The function `parseColorString` takes a hexadecimal color string, extracts the RGB values, and
 * returns a CRGB object.
 *
 * @param colorString The `colorString` parameter is a string representing a color in hexadecimal
 * format. It seems like the function expects the colorString to be in the format "#RRGGBB" where RR
 * represents the red component, GG represents the green component, and BB represents the blue
 * component. The function then parses
 *
 * @return The `parseColorString` function returns a CRGB object with the RGB values extracted from the
 * input colorString.
 */
CRGB parseColorString(String colorString)
{
  colorString.remove(0, 1);

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

  DynamicJsonDocument json(4048);
  DeserializationError err = deserializeJson(json, dataFile);
  if (err)
  {
    Serial.println("Error");
    USE_SERIAL.printf(err.c_str());
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
  interactDelay = json["config"]["interactDelay"].as<unsigned long>();

  loadColorModeConfig(json);

  delayCategoryColor = delayColor;

  FastLED.setBrightness(brightness);

  for (const auto &shelf : shelfs)
  {

    int shelfIndex = shelf["shelfIndex"].as<int>();
    clearLEDStripToApplyConfig(shelfIndex);

    int segmentsNumber = shelf["segmentsNumber"].as<int>();

    int iIndex = 0;

    for (int i = 0; i < segmentsNumber; i++)
    {

      String segmentColor = shelf["segments"][i]["color"].as<String>();

      int segmentSize;

      if (i == segmentsNumber - 1 && segmentsNumber > 1)
      {
        segmentSize = pixels;
      }
      else
      {
        segmentSize = shelf["segments"][i]["size"].as<int>();
      }

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

void loadColorModeConfig(const DynamicJsonDocument &json)
{
  colorMode = json["config"]["colorMode"].as<std::string>();
  defaultWarmColor = json["config"]["defaultWarmColor"].as<std::string>();
  defaultColdColor = json["config"]["defaultColdColor"].as<std::string>();
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

void clearAllStrips()
{
  fill_solid(leds[0], NUM_LEDS, CRGB::Black);
  fill_solid(leds[1], NUM_LEDS, CRGB::Black);
  fill_solid(leds[2], NUM_LEDS, CRGB::Black);
  fill_solid(leds[3], NUM_LEDS, CRGB::Black);
  fill_solid(leds[4], NUM_LEDS, CRGB::Black);
  fill_solid(leds[5], NUM_LEDS, CRGB::Black);
  fill_solid(leds[6], NUM_LEDS, CRGB::Black);
  fill_solid(leds[7], NUM_LEDS, CRGB::Black);
}

void clearStrip(std::string mode)
{

  uint8_t currentBrightness = getConfigBrightness();
  CRGB defaultColor = getDefaultColor();
  int step = getFadeStep(currentBrightness);

  for (int i = currentBrightness; i >= 0; i -= step)
  {

    FastLED.setBrightness(i);
    FastLED.show();
  }

  FastLED.setBrightness(0);
  FastLED.show();

  clearAllStrips();

  if (mode == "exit_config_mode")
  {
    fadeInAllSegments(defaultColor);
    USE_SERIAL.println("exit_config_mode");
  }

  if (mode == "products_fade")
  {
    applyMidiaColor();
    USE_SERIAL.println("products_fade");
  }

  if (mode == "config_mode"){
    setLEDStripProperties();
  }



  for (int i = 0; i <= currentBrightness; i += step)
  {
    FastLED.setBrightness(i);
    FastLED.show();
  }
}

/**
 * The function takes a string representing an RGB color and returns a vector of integers representing
 * the individual color components.
 *
 * @param rgbString The `rgbString` parameter is a `std::string` that represents an RGB color value in
 * the format "R,G,B".
 */
std::vector<int> parseRGBString(const std::string &rgbString)
{
  std::vector<int> rgbValues;

  if (rgbString.substr(0, 4) == "rgb(" && rgbString.back() == ')')
  {

    std::string valuesString = rgbString.substr(4, rgbString.size() - 5);

    std::stringstream ss(valuesString);
    std::string token;

    while (std::getline(ss, token, ','))
    {
      try
      {
        // Convert the token to an integer and add it to the vector
        int value = std::stoi(token);
        rgbValues.push_back(value);
      }
      catch (const std::invalid_argument &e)
      {
        // Handle invalid arguments (non-integer values)
        std::cerr << "Invalid RGB value: " << token << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "Invalid RGB string format: " << rgbString << std::endl;
  }

  return rgbValues;
}