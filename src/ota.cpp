#include "settings.h"

    // OTA update code
    #include <AsyncElegantOTA.h>
    #include <AsyncTCP.h>
    #include <ESPAsyncWebServer.h>
    #include <WiFi.h>
    
    const char* ssid = "pelan";
    const char* password = "Datsun240z";
    // End OTA update
    // OTA Server
    AsyncWebServer server(80);

void setUpOta(){
   WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", version );});  

  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("Elegant OTA Initiated");
  Serial.println("HTTP Server Has started Sucessfully");
  Serial.println("To access OTA Update, type");
  Serial.println(WiFi.localIP());
  Serial.println("/update");    
}