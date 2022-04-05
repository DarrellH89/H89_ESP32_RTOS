/*
    Micronics Technology H89 ESP32 intrface
    Copyright (C) 2022  Darrell Pelan

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "settings.h"
#include "license.h"


AsyncWebServer *server;  
extern Config config;
extern bool shouldReboot;
const char* menuStr = "Menu\n v: Prints version\n b: Reboots system\n r: Resets counters\n s: SD card test\n c: Clears NVM\n m: Prints menu\n";
// Test LED blinkers 
const byte led1 = ALIVE_LED;
unsigned long delayLed = 0;
volatile int t1 = 0;

//************** H89 data flags and buffer
volatile int currentStatus = 0 ;          // status value for H89 to read
volatile int h89ReadData = DATA_SENT ;    // status value for H89 data actually read
volatile int h89BytesToRead = 0;

volatile byte data[256] ;
volatile int dataPtr = 0;

//************* Interupt Counters
volatile int intr7C_cnt = 0;
volatile int  intr7E_cnt = 0;
volatile int  intr7CRead_cnt = 0;
int last7C = 0;
//int last7D = 0;
int last7E = 0;
int last7CRead = 0;

portMUX_TYPE Cmdmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE DataInmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE DataOutmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// port pin definitions D0..D7
volatile int pins[] = {32, 33,25,26,27,14,12,13};
// Current data direction 
volatile int pinInOut = DATA_OUT;
// Command control bytes
volatile byte cmdData[10];
volatile byte cmdDataPtr = 0 ;
volatile int8_t cmdFlag = 0;

// Data out bytes
volatile byte dataOutBuf[1024];
volatile int dataOutBufPtr = 0;
volatile int dataOutBufLast = 0;

// ddebugging variables
volatile int dataInTime[100];
volatile int dataInTimePtr ;
volatile int bitCtr = 0;
volatile int bits[100];
int offset = 1;
int sent[20], sentPtr;

// Debouncing parameters
long debouncing_time = 1000; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;
//volatile byte dataOutFlag = 1 ;


/********************************* Delay interrupt timer ***************************/
volatile int interruptCounter;
int totalInterruptCounter;
  
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

// ************************************ Interrupt Hanedler H89 Write 7C *************************************
void IRAM_ATTR intrHandle7C() {     // Data flag
  portENTER_CRITICAL_ISR(&mux);
   setStatusPort(ESP_BUSY);
//  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
//  digitalWrite(ISR_AWAKE, LOW);
//  last_micros = micros();

  intr7C_cnt++;
  if((cmdFlag == 1) && (cmdDataPtr < CMD_LENGTH)){
    cmdData[cmdDataPtr++] = dataIn();
    //ets_printf("cmd %d\n", cmdData[cmdDataPtr-1]);
  }
  else {
    data[dataPtr++] = dataIn();
    }   
  if(cmdDataPtr == CMD_LENGTH)  
    cmdFlag = 0;
  setStatusPort(H89_WRITE_OK);

 //   ets_printf("dataOut Error: %d\n", errCnt);
    // ataPtr, (long)micros()-last_micros);        // Safe to use in an interrupt routine
    //cmdFlag = 4;
 // digitalWrite(ISR_AWAKE, HIGH);
  portEXIT_CRITICAL_ISR(&mux);
}
// ************************************ Interrupt Hanedler H89 Read 7C *************************************
void IRAM_ATTR intrHandleRead7C() {     // Data flag
  portENTER_CRITICAL_ISR(&mux);
  h89ReadData =  H89_GOT_DATA;
  intr7CRead_cnt++;
  if(h89BytesToRead >0){
      h89BytesToRead--;
    if(h89BytesToRead == 0)  
      setStatusPort(CMD_RDY);
  }
    portEXIT_CRITICAL_ISR(&mux);
}
// ************************************ Interrupt Hanedler H89 Write 7E *************************************
void IRAM_ATTR intrHandle7E() {     // Command flag
  portENTER_CRITICAL_ISR(&Cmdmux);
  //if((long)(micros() - last_micros) >= debouncing_time * 1000) {
  // data is coming so set data lines for input
  setInput();
  setStatusPort(H89_WRITE_OK);

  intr7E_cnt++;
  cmdDataPtr = 0;
  cmdFlag = 1;

  portEXIT_CRITICAL_ISR(&Cmdmux);
}


// ************************************ Setup *************************************
void setup() {
  Serial.begin(115200);
  Serial.println(copyRightNoticeShort);
  dataInTimePtr = 0;
  setupWifi();
  Serial.println("Configuring Webserver ...");
  server = new AsyncWebServer(WEB_SERVER);
  configureWebServer();  
  if(!SD.begin()){     //SD_CS,spi,80000000)){
    Serial.println("Card Mount Failed");
    return;
}

   // H89 interface setup
  setPorts();
  setInput();
  pinInOut = DATA_IN;         // data port pins flag

  pinMode(led1,OUTPUT);       // Keep alive LED

  attachInterrupt(digitalPinToInterrupt(intr7C), intrHandle7C, FALLING);
  attachInterrupt(digitalPinToInterrupt(intr7E), intrHandle7E, FALLING);
  attachInterrupt(digitalPinToInterrupt(H89_READ_DATA), intrHandleRead7C, FALLING);
  cmdFlag = CMD_RDY;
  setStatusPort(cmdFlag);
 
  Serial.println(version);
  Serial.printf("Status %d\n", cmdFlag);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
  // debug stuff
  Serial.printf("Heap: Free %d, Min: %d, Size: %d, Alloc: %d\n", ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
  sentPtr = -1;
  Serial.printf(menuStr);
}

//************** LOOP ****************

void loop() {
  int sendCnt = 0;
  long errCnt = 0 ;
  // Test Web Socket
    // cleanSocket();
  if (shouldReboot) {
    rebootESP("Web Admin Initiated Reboot");
  }
  // termninal interaction code
  if(Serial.available() > 0) {
    String dataStr = Serial.readString();
    switch(tolower(dataStr[0])){
      case 'v':
        Serial.println(version);
        break;
      case 'm':
        Serial.println(menuStr);
        break;
      case 'l':
        Serial.println(Notice);
        break;        
      case 'b':
        Serial.println("Restart\n"); 
        ESP.restart();
        break;
      case 'r':
        Serial.println("Resetting counters\n");
        last7C = intr7C_cnt = last7E = intr7E_cnt = 0;
        bitCtr = 0;
        t1 = 0;
        break;
      case 's':
        Serial.println("SD Card test\n");
        sdTest();
        break; 
      case 'c':
        Serial.println("Clear NVM\n");
        setConfig(true);      
        break;
      }
    }
  // Check if all command bytes arrived
  if (cmdDataPtr >= CMD_LENGTH){
    // load data to send back
    for(sendCnt = 0; sendCnt < CMD_LENGTH; sendCnt++)
      dataOutBuf[dataOutBufPtr+sendCnt] = cmdData[dataOutBufPtr+sendCnt]+offset;
    offset++;
    dataOutBufLast = dataOutBufPtr + sendCnt  ;
    // Send 4 bytes of buffer data
    h89BytesToRead = 4;
    h89ReadData = DATA_SENT;
    Serial.printf(" Buffer Last %d, Buffer Ptr %d\n", dataOutBufLast, dataOutBufPtr);
    if(dataOutBufLast - dataOutBufPtr == 4){
      int temp = dataOutBufPtr;
      while(temp < dataOutBufLast){
        if(dataOut(dataOutBuf[temp]) == 0 ){
          //sent[++sentPtr] = dataOutBuf[temp];
          temp++;
          }
        else 
          errCnt++;  
        }
     }  
    // print cmd bytes received
    for(int i = 0; i < CMD_LENGTH; i++){
      Serial.printf("Cmd Byte %d\n", cmdData[i]);
      cmdData[i] = 0;                 // reset command buffer
    }
    cmdDataPtr = 0;

    // print data buffer
 // debug
    // if(sentPtr >=0){
    //   int i = 0;
    //   while (i <= sentPtr)
    //   {
    //     Serial.printf("Sent %d\n",sent[i++]); 
    //   }
    // sentPtr = -1;
    // }
// end debug
    while(dataOutBufPtr < dataOutBufLast){
      Serial.printf("Sent %d\n",dataOutBuf[dataOutBufPtr]);
      dataOutBuf[dataOutBufPtr] = 0;
      dataOutBufPtr++;
      }
    dataOutBufPtr = 0 ;  
    dataOutBufLast = 0;
    if(errCnt > 0){
      Serial.printf("Data Out errors: %ld\n", errCnt);
      errCnt = 0;
      } 

    }
   if(intr7C_cnt - last7C >3){
    Serial.printf("Interrupt 7C count = %d\n", intr7C_cnt);
    //Serial.printf("Cmd Ptr: %d Data Ptr: %d\n", cmdDataPtr, dataPtr);
    if(dataPtr > 0){
      Serial.println("Data Received"); 
      for(int i = 0; i < dataPtr; i++){
        Serial.printf("%d\n",data[i]);
        }
      portENTER_CRITICAL(&DataInmux);
      dataPtr = 0;  
      portEXIT_CRITICAL(&DataInmux);
      }
    last7C = intr7C_cnt ;
    }

  if(intr7E_cnt > last7E){
    Serial.print("Interrupt 7E count = ");
    Serial.println(intr7E_cnt);
    last7E = intr7E_cnt ;
    }
  if(intr7CRead_cnt > last7CRead){
    Serial.print("Interrupt 7C Read count = ");
    Serial.println(intr7CRead_cnt);
    last7CRead = intr7CRead_cnt ;
    }
  if(interruptCounter > 0){
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
 
    totalInterruptCounter++;
    if(totalInterruptCounter % 2 == 0)
        digitalWrite(led1,LOW);
      else
        digitalWrite(led1,HIGH);
  }

}