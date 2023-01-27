#include <Arduino.h>
#include <WiFi.h>


const char* SSID = "";
const char* password = "";


void setup() {
  Serial.begin(9600);

  Wifi.begin(SSID, password);

  While(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print()
  }

}

void loop() {
  
  Serial.println("I can fix it");
  delay(1000);
}