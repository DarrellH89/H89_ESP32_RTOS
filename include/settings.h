#ifndef SETTINGS_H
    #define SETTINGS_H
    #include <Arduino.h>
    #include "fs.h"
    #include "sd.h"
    #include "spi.h"
    #include <Wire.h> // must be included here so that Arduino library object file references work
    #include <AsyncTCP.h>
    #include <ESPAsyncWebServer.h>
    #include <WiFi.h>
    #include <Preferences.h>


   //******** Webserver related variables
   #define FIRMWARE_VERSION "v0.0.1"
    struct Config {
        String ssid;               // wifi ssid
        String wifipassword;       // wifi password
        String httpuser;           // username to access web admin
        String httppassword;       // password to access web admin
    } ;

    //************* Server ports
    #define WEB_SERVER 80
    // #define FILE_SERVER 81

   //******** End Websrvr related variables ***********

    //************* New Data & Status Out pins
 
    #define DATA_IN_OE      2
    #define DATA_OUT_OE     15
    #define STATUS_BIT_1    4
    #define STATUS_BIT_0    0
    #define ALIVE_LED       22
    //***************** Data direction flag values
    #define DATA_OUT        0
    #define DATA_IN         1

    #define CMD_LENGTH      40
    #define BUFFER_LEN      1024
    //***************** Interrupt Pins
    #define intr7C          35  
    #define intr7E          34    
    #define H89_READ_DATA   21
    //*************** H89 Status Port values also written to currentStatus
   // #define MAX_READ_WAIT   10000
    #define CMD_RDY         0b00000000
    #define H89_READ_OK     0b00000001
    #define H89_WRITE_OK    0b00000010   
    #define ESP_BUSY        0b00000011
    //*************** H89 Read/Write values
    #define DATA_NOT_READ   4
    #define DATA_SENT       5
    // #define H89_OK_TO_READ  1
    // #define H89_GOT_DATA    2
    #define TIMEOUT         3000             // milli seconds
    #define HOLD -1                             // disable timeOutCounter

    const String version = "Hi! I am H89-ESP32, Version 3.0 E 5/3/22";

    //**************** SD Card pins
    #define SD_CLK  18
    #define SD_OUT  23      // MOSI - Microcontroller Out Serial In pin
    #define SD_IN   19      // MISO -  Microcontroller In Serial Out pin
    #define SD_CS   5
    //************** function definitions
    // port management
    void setPorts();
    void setOutput();
    void setInput();
    // Data In / Out
    byte dataOut(byte data);
    int dataIn();
    void setStatusPort(byte status);
    // Over the Air
    void setUpOta();
    // File Test routine functions
    void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
    void createDir(fs::FS &fs, const char * path);
    void removeDir(fs::FS &fs, const char * path);
    void readFile(fs::FS &fs, const char * path);
    void writeFile(fs::FS &fs, const char * path, const char * message);
    void appendFile(fs::FS &fs, const char * path, const char * message);
    void renameFile(fs::FS &fs, const char * path1, const char * path2);
    void deleteFile(fs::FS &fs, const char * path);
    void testFileIO(fs::FS &fs, const char * path);
    
    // End File Test Functions

    bool setupWifi();
    void setupFileServer();
    void sdTest() ;

    //void initWebSocket();
    void cleanSocket();
    String humanReadableSize(const size_t bytes);
    void rebootESP(String message) ;
    void configureWebServer() ;
    String listFiles(bool ishtml);
    void rebootESP(String message); 
    String processor(const String& var);
    void notFound(AsyncWebServerRequest *request);
    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    bool checkUserWebAuth(AsyncWebServerRequest * request);
    void notFound(AsyncWebServerRequest *request) ;
    void configureWebServer();
    void rebootESP(String message) ;
    String listFiles(bool ishtml) ;
    bool setConfig(bool reset);
    void commands();
    void sendH89String(String sendIt);
    String getH89FileName();
    bool getH89File(String fname);
    bool getData(byte &x);
    int getDataTime( byte &x, int time);
    int sendDataTime( byte x, int time);
    int getH89Int();
    void printDataBufPtr();
    uint16_t calcrc(byte *ptr, int count);
#endif  