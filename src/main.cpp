#include <Arduino.h>

#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFi.h>

#include <SPIFFS.h>
#include <FFat.h>

void handleRoot();
void handleNotFound();
String htmlPage();
void startConfigWebpage();

const char* SSID = "StMarche";
const char* password = NULL;

uint8_t builtinLed = 5;  


WiFiClient client;
WebServer server(80);


void setup() {
  Serial.begin(115200);

  Serial.println("\n[+] Creating Access Point...");
  pinMode(builtinLed, OUTPUT);

  if (!FFat.begin(true)){
    Serial.println("Couldn't mount File System");
  }

  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SSID, password);

  Serial.print("[+] AP Created with IP Gateway ");


  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  while (true)
  {
    server.handleClient();
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
  // server.send(200, "text/html", htmlPage());
  file.close();
  SPIFFS.end();
}

void handleNotFound() {
  Serial.println("Handle not Found");
  // server.send(200, "text/html", htmlPage());
  server.send(404,"text/plain", "404: Not found");
}


String htmlPage() {
  String content = "<!DOCTYPE html>";
  content +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  content +="<title>LED Control</title>\n";
  content += "</head>\n";
  content +="<body>\n";
  content +="<h1>ESP32 Web Server</h1>\n";
  content +="<h3>Using Access Point(AP) Mode</h3>\n";
  content +="</body>\n";
  content +="</html>\n";

  return content;
}

