#ifndef SETTINGS_H
    #define SETTINGS_H
    #include <Arduino.h>

//    static bool dataOutFlag = true ;
//    static bool testFlag = true ;
//    static int ports[] = {32,33,25,26,27,14,12,13};
   // OE Controls - Active low
    // #define DATA_IN_OE 4               // IO 4
    // #define DATA_IN_CLEAR 0            // Clear data in flip flop
    // LE Controls - high to load dat, low to latch
    // #define DATA_OUT_LE 15              // IO 15
    // #define STATUS_LE 2                  // IO 2
    // Input signals
    //#define READ_DATA_FLAG 4           //      IO15
    // ************ Data & Status Out pins
    // #define SERIAL_OUT      13
    // #define SERIAL_IN       12

    // #define DATA_SRCLK      25
    // #define DATA_RCLK       26
    // Data in pins
    //#define H89_LOAD        25
    // #define H89_CLOCK_IN    27
    // #define H89_CLOCK_ENABLE    14
    //************* New Data & Status Out pins
    #define H89_READ_DATA   39//15
    #define DATA_IN_OE      2//16
    #define DATA_OUT_OE     15//17
    #define STATUS_BIT_1    4
    #define STATUS_BIT_0    0
    #define ALIVE_LED       5
    //***************** Data direction flag values
    #define DATA_OUT        0
    #define DATA_IN         1

    //************* Feedback pin
    //#define ISR_AWAKE       2
    #define CMD_LENGTH      4
    //***************** Interrupt Pins
    #define intr7C          35   // ESP32 pin I15
    #define intr7E          34     // ESP32 pin IO2

    #define MAX_READ_WAIT   10000
    #define CMD_RDY         0b00000000
    #define H89_READ_OK     0b00000001
    #define H89_WRITE_OK    0b00000010   
    #define ESP_BUSY        0b00000011
    #define DATA_NOT_READ   2
    #define DATA_SENT        0

    const String version = "Hi! I am H89-ESP32, Version 2.0 A";

    // const byte led3 = 17;
    // const byte led4 = 2;

    // #include <TFT_eSPI.h>
    // #include "AdafruitIO_WiFi.h"
    // #include "network.h"
    // #include <Preferences.h>
    // #include <esp_system.h>
    // #include "mqtt_controller.h"

    // #include "ezTime.h"
    // #include "clock.h"
    void setPorts();
    void setOutput();
    void setInput();
    // void setOutput();
    // void setOtherPort();
    byte dataOut(byte data);
    byte dataIn();
    void setStatusPort(byte status);
    void setUpOta();
    // void latchDataOut(byte data);
    //void dataTo595( byte data);
#endif  