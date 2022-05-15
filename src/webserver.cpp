#include "settings.h"
#include <AsyncElegantOTA.h>
#include "webpages.h"
#define RW_MODE false
#define RO_MODE true

String logmessage = "";
// configuration structure
Config config;

extern AsyncWebServer *server;  
//Config config;                        // configuration
bool shouldReboot;
// const String default_ssid = "pelan";
// const String default_wifipassword = "Datsun240z";
// const String default_httpuser = "admin";
// const String default_httppassword = "admin";
// const int default_webserverporthttp = 80;
Preferences prefs;              // access to EEPROM (NVM)


String askInfo(String what){
  String dataStr = "";
  bool test = false;
  while(!test){
    Serial.printf("Please enter %s (15 char or less): ", what.c_str());
    while(Serial.available() == 0) ;
    Serial.println();
    dataStr = Serial.readStringUntil('\n');
    if(dataStr.length() < 15 )
      test = true;
    }
  return dataStr ;  
}
//***************** set Config
// Get network parameters from NVM
bool setConfig(bool reset){
  int check = 0;
  bool okay = false;

  if(reset){            // kills the config namespace
    prefs.begin("config",RW_MODE);
    prefs.clear();
    prefs.end();
    return okay;
  }

  Serial.println("Loading Configuration ...");

  while(!okay && check++ < 5){             // Try Five times to get the password data
    Serial.printf("Checking Data\n");
    prefs.begin("config",RW_MODE);
    if(!prefs.isKey("ssid")){
      Serial.println("ssid key NOT okay, ask for data");
      config.ssid = askInfo("ssid");
      config.wifipassword = askInfo("wifi password");
      config.httpuser = askInfo("web user id");
      config.httppassword = askInfo("web password");
      
      prefs.putString("ssid", config.ssid);
      prefs.putString("pw", config.wifipassword);
      prefs.putString("user", config.httpuser);
      prefs.putString("httppw", config.httppassword);
      Serial.printf("perf ssid: %s\n", prefs.getString("ssid", "crap").c_str());
    } else{
            Serial.println("ssid key okay");
            Serial.print("ssid: ");Serial.println(prefs.getString("ssid",""));
        config.ssid = prefs.getString("ssid","");
        config.wifipassword = prefs.getString("pw","");
        config.httpuser = prefs.getString("user","");
        config.httppassword = prefs.getString("httppw","");
        }
    if(config.ssid.length() > 1 &&
       config.wifipassword.length() > 1 &&
       config.httpuser.length() > 1 &&
       config.httppassword.length() > 1)
        okay = true;
    prefs.end();    
  }
  //Serial.printf("ssid %s, pw %s, http user %s, http pw: %s\n", config.ssid.c_str(), config.wifipassword.c_str(), config.httpuser.c_str(), config.httppassword.c_str());
  prefs.end();  
  return okay;
 }

//************ Setup WiFi *************
bool setupWifi(){
  bool result = false;
  int attempts = 20;
  int cnt = 0;
  String errorType[] = {"0", "No SSID Available", "Scan Completed", "Connected", "Connection Failed","Connection Lost","Disconnected"};
  Serial.println("\nWiFi Configuration ...");
  if(setConfig(false)){
    Serial.print("\nConnecting to Wifi: \n");
    WiFi.begin(config.ssid.c_str(), config.wifipassword.c_str() );
    while (WiFi.status() != WL_CONNECTED && cnt++ < attempts) {
      delay(500);
      Serial.print(".");
      Serial.print("Failed with state: "); Serial.println(WiFi.status());
      }
    if(cnt > attempts){
        Serial.printf("\nWiFi failed to connect\n");
      }
      else{
        Serial.println("\n\nNetwork Configuration:");
        Serial.println("----------------------");
        Serial.print("         SSID: "); Serial.println(WiFi.SSID());
        Serial.print("  Wifi Status: "); Serial.println(WiFi.status());
        Serial.print("Wifi Strength: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
        Serial.print("          MAC: "); Serial.println(WiFi.macAddress());
        Serial.print("           IP: "); Serial.println(WiFi.localIP());
        Serial.print("       Subnet: "); Serial.println(WiFi.subnetMask());
        Serial.print("      Gateway: "); Serial.println(WiFi.gatewayIP());
        Serial.print("        DNS 1: "); Serial.println(WiFi.dnsIP(0));
        Serial.print("        DNS 2: "); Serial.println(WiFi.dnsIP(1));
        Serial.print("        DNS 3: "); Serial.println(WiFi.dnsIP(2));
        Serial.println();
        result = true;
      }
  }
  return result;  
}

//************ Setup OTA Updates
void setUpOta(){
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(200, "text/plain", version );});  

  AsyncElegantOTA.begin(server);
  server->begin();
  Serial.println("Elegant OTA Initiated");
  Serial.println("HTTP Server Has started Sucessfully");
  Serial.println("To access OTA Update, type");
  Serial.print(WiFi.localIP());
  Serial.println("/update");    
}

// ******************* parses and processes webpages
// if the webpage has %SOMETHING% or %SOMETHINGELSE% it will replace those strings with the ones defined
String processor(const String& var) {
  if (var == "FIRMWARE") {
    return FIRMWARE_VERSION;
  }

  if (var == "FREESD") {
    return humanReadableSize((SD.totalBytes() - SD.usedBytes()));
  }

  if (var == "USEDSD") {
    return humanReadableSize(SD.usedBytes());
  }

  if (var == "TOTALSD") {
    return humanReadableSize(SD.totalBytes());
  }
  return "";
}

//**************** Configure Web Server *********************
void configureWebServer() {
  // configure web server

  // if url isn't found
  server->onNotFound(notFound);

  // run handleUpload function when any file is uploaded
  server->onFileUpload(handleUpload);

  // visiting this page will cause you to be logged out
  server->on("/logout", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->requestAuthentication();
    request->send(401);
  });

  // presents a "you are now logged out webpage
  server->on("/logged-out", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);
    request->send_P(401, "text/html", logout_html, processor);
  });

  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();

    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      request->send_P(200, "text/html", index_html, processor);
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      return request->requestAuthentication();
    }

  });

  server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

    if (checkUserWebAuth(request)) {
      request->send(200, "text/html", reboot_html);
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      shouldReboot = true;
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/listfiles", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      request->send(200, "text/plain", listFiles(true));
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      return request->requestAuthentication();
    }
  });

  server->on("/file", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (checkUserWebAuth(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);

      if (request->hasParam("name") && request->hasParam("action")) {
        const char *fileName = request->getParam("name")->value().c_str();
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

        if (!SD.exists(fileName)) {
          Serial.println(logmessage + " ERROR: file does not exist");
          request->send(400, "text/plain", "ERROR: file does not exist");
        } else {
          Serial.println(logmessage + " file exists");
          if (strcmp(fileAction, "download") == 0) {
            logmessage += " downloaded";
            request->send(SD, fileName, "application/octet-stream");
          } else if (strcmp(fileAction, "delete") == 0) {
            logmessage += " deleted";
            SD.remove(fileName);
            request->send(200, "text/plain", "Deleted File: " + String(fileName));
          } else {
            logmessage += " ERROR: invalid action param supplied";
            request->send(400, "text/plain", "ERROR: invalid action param supplied");
          }
          Serial.println(logmessage);
        }
      } else {
        request->send(400, "text/plain", "ERROR: name and action params required");
      }
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      return request->requestAuthentication();
    }
  });
  setUpOta();
}

//************** Client not found
void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);
  request->send(404, "text/plain", "Not found");
}

//********************* used by server.on functions to discern whether a user has the correct httpapitoken OR is authenticated by username and password
bool checkUserWebAuth(AsyncWebServerRequest * request) {
  bool isAuthenticated = false;

  if (request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
    Serial.println("is authenticated via username and password");
    isAuthenticated = true;
  }
  return isAuthenticated;
}

//******************** handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // make sure authenticated before allowing upload
  if (checkUserWebAuth(request)) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);

    if (!index) {
      logmessage = "Upload Start: " + String(filename);
      // open the file on first call and store the file handle in the request object
      request->_tempFile = SD.open("/" + filename, "w");
      Serial.println(logmessage);
    }

    if (len) {
      // stream the incoming chunk to the opened file
      request->_tempFile.write(data, len);
      logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
      Serial.println(logmessage);
    }

    if (final) {
      logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
      // close the file handle as the upload is now done
      request->_tempFile.close();
      Serial.println(logmessage);
      request->redirect("/");
    }
  } else {
    Serial.println("Auth: Failed");
    return request->requestAuthentication();
  }
}

//******************** Reboot ESP *******************
void rebootESP(String message) {
  Serial.print("Rebooting ESP32: "); Serial.println(message);
  ESP.restart();
}

//************** List Files *********************************
// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  int cnt = 1;
  Serial.println("Listing files stored on SD");
  File root = SD.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    } else {
      returnText += String(cnt) +": "+ String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
      cnt++;
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

//************************** Human Readable Size **********************
// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
