#ifndef SETTINGS_H
    #define SETTINGS_H
    #include <Arduino.h>
  
    const byte led1 = 32;
    const byte led2 = 4;
    const byte  intr7C = 15;   // ESP32 pin I15
    const byte intr7D = 2 ;    // ESP32 pin IO2
    const byte intr7E = 0;     // ESP32 pin I00
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
    void setInput();
    void setOutput();
    void setOtherPort();
#endif  