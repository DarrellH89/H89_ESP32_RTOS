#ifndef SETTINGS_H
    #define SETTINGS_H
    #include <Arduino.h>
    
    //************* New Data & Status Out pins
 
    #define DATA_IN_OE      2
    #define DATA_OUT_OE     15
    #define STATUS_BIT_1    4
    #define STATUS_BIT_0    0
    #define ALIVE_LED       21
    //***************** Data direction flag values
    #define DATA_OUT        0
    #define DATA_IN         1

    #define CMD_LENGTH      4
    //***************** Interrupt Pins
    #define intr7C          35  
    #define intr7E          34    
    #define H89_READ_DATA   5

    //*************** Status values
    #define MAX_READ_WAIT   10000
    #define CMD_RDY         0b00000000
    #define H89_READ_OK     0b00000001
    #define H89_WRITE_OK    0b00000010   
    #define ESP_BUSY        0b00000011
    #define DATA_NOT_READ   2
    #define DATA_SENT       0
    #define H89_OK_TO_READ  1
    #define H89_GOT_DATA    2

    const String version = "Hi! I am H89-ESP32, Version 3.0 A";

    //************** function definitions
    void setPorts();
    void setOutput();
    void setInput();

    byte dataOut(byte data);
    int dataIn();
    void setStatusPort(byte status);
    void setUpOta();
#endif  