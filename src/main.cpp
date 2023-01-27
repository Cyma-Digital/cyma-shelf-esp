#include <Arduino.h>
#include <WiFi.h>


const char* SSID = "StMarche";
const char* password = "12345678";


void setup() {
  Serial.begin(115200);

  Serial.println("\n[+] Creating Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID, password);

  Serial.print("[+] AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());

  // WiFi.begin(SSID, password);

  // while(WiFi.status() != WL_CONNECTED){
  //   Serial.println(WiFi.status());
  //   delay(500);
  //   Serial.println("Connecting to Wi-FI...");
  // }
  // Serial.println(WiFi.status());
  // Serial.println("Connected to the Wi-FI network...");
  // Serial.println(WiFi.localIP());

}

void loop() {}