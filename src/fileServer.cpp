#include "settings.h"
#include "fileserver.h"
    // OTA update code
#include <AsyncElegantOTA.h>

extern const char* ssid ;
extern const char* password ;
const char* fileversion = "File Server version 0.2";

// OTA Server
AsyncWebServer server(OTA_SERVER);
//AsyncWebSocket ws("/ws");

void setUpOta(){


//   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
//     request->send(200, "text/plain", version );});  

  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("Elegant OTA Initiated");
  Serial.println("HTTP Server Has started Sucessfully");
  Serial.println("To access OTA Update, type");
  Serial.print(WiFi.localIP());
  Serial.println("/update");    
}
// AsyncWebServer fileServer(FILE_SERVER);

// class h89FileServerClass {
//     public:
//         void begin(AsyncWebServer *server, const char* username = "", const char* password = ""){
//             _server = server;
//         }
       
//     private:
//         AsyncWebServer *_server ;
// };
// h89FileServerClass h89FileServer;

// String geth89upload();
//*********************** Test
bool ledState = 0;

void notifyClients() {
//  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      Serial.println("Toggle");  
      ledState = !ledState;
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *tserver, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}
void initWebSocket() {
  // ws.onEvent(onEvent);
  // server.addHandler(&ws);
  //   // Route for root / web page

  // server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/html", index_html, processor);
  // });

}

void  cleanSocket(){
//    ws.cleanupClients();
}

// //******************** h89 File Server 

// void appendF(fs::FS &fs, const char * path, const char * message){
//   File file = fs.open(path, FILE_APPEND);
//   if(!file){
//     Serial.println("Failed to open file for appending");
//     return;
//   }
//   if(!file.print(message)){
//      Serial.println("Append failed");
//   }
//   file.close();
// }

//*************** int Comma ***************
// adds commas to number strings
String intComma(String s){
    int l = s.length();
    String result = "";
    int first = l - 3;
    
    if(first < 0)       // string less than 3 chars, no change needed
        return s;

    while(first > 0){
        result =  ","+s.substring(first, l) + result;
        l = first;
        first = l-3;
    }
    result = s.substring(0, l)+result;
    return result;
}

//*********** dir Txt
void dirTxt(String fn){
    File root =SD.open("/");
    if(!root){
        Serial.println("Failed to open directory");
        return;
        }
    File dirfile = SD.open(fn, FILE_WRITE);
    dirfile.printf("SD Card Directory list \n\n");
    File file = root.openNextFile();
    int cnt = 1;
    while(file){
        String fname = file.name();
        String fsize = intComma(String(file.size()));
        char const *fsizeptr = fsize.c_str();
        char spacer[50] ;
        int i ;
        for( i = 0; i <40-fname.length()+10-fsize.length(); i++ ) 
            spacer[i]= ' ';
        spacer[i] = 0;
        dirfile.printf("%3d. %s %s %s\n", cnt++, file.name(), spacer,fsizeptr);
        file = root.openNextFile();
    }    
    dirfile.close();
}
//********************* Setup File Server
void setupFileServer(){

//   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
//     request->send(200, "text/plain", fileversion );
//     });  
  initWebSocket();
  server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request){
    //String xml = "<!DOCTYPE html> <html lang=""en""> <head> <meta charset=""UTF-8"">   <title>Document</title> </head><body>";  
    if(!SD.begin()){     //SD_CS,spi,80000000)){
        Serial.println("Card Mount Failed");
        return;
        }   
    Serial.println("Reading File list");
    dirTxt("/dir.txt");
    Serial.println(" Done Reading File list");
    request->send(SD, "/dir.txt", "text/plain", false);
    });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
                // AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", );
                // response->addHeader("Content-Encoding", "gzip");
                request->send(200, "text/html", h89File_html);
            });
  server.on("/upload", HTTP_POST, [&](AsyncWebServerRequest *request) {  
       request->send(200, "text/html", "hi");
      Serial.println("Got upload request");
  });          
    // Start ElegantOTA
    setUpOta();
 // AsyncElegantOTA.begin(&server);
    // Start server
  //server.begin();
//  server.begin(&fileServer);
//   //fileServer.handleClient();
//   fileServer.begin();
  Serial.println("H89 File Server Initiated");
  Serial.println("HTTP Server Has started Sucessfully");
  Serial.println("To access H89 File Server, type");
  Serial.print(WiFi.localIP());
  Serial.printf(":%d", FILE_SERVER);
  Serial.println("/list to display file list");    
  Serial.println("/upload to upload file");    
  Serial.println("/download to download a file");    
  
}
