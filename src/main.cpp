#include "settings.h"
//#include "global.hpp"

// Test LED blinkers 
const byte led1 = ALIVE_LED;
unsigned long delayLed = 0;
volatile int t1 = 0;
volatile int currentStatus = 0 ;
volatile byte data[256] ;
volatile int dataPtr = 0;
volatile int intr7C_cnt = 0;
volatile int  intr7E_cnt = 0;
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

// Debouncing parameters
long debouncing_time = 1000; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;
//volatile byte dataOutFlag = 1 ;

int last7C = 0;
int last7D = 0;
int last7E = 0;

portMUX_TYPE Cmdmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE DataInmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE DataOutmux = portMUX_INITIALIZER_UNLOCKED;
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
  portENTER_CRITICAL_ISR(&DataInmux);
   setStatusPort(ESP_BUSY);
//  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
//  digitalWrite(ISR_AWAKE, LOW);
  last_micros = micros();

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
  portEXIT_CRITICAL_ISR(&DataInmux);
}
// ************************************ Interrupt Hanedler H89 Write 7C *************************************
void IRAM_ATTR intrHandleRead7C() {     // Data flag
  portENTER_CRITICAL_ISR(&DataOutmux);
  if(dataOutBufPtr >=0)
    setStatusPort(ESP_BUSY );  
  else
    setStatusPort(CMD_RDY)  ;
  portEXIT_CRITICAL_ISR(&DataOutmux);
}
// ************************************ Interrupt Hanedler H89 Write 7E *************************************
void IRAM_ATTR intrHandle7E() {     // Command flag
  portENTER_CRITICAL_ISR(&Cmdmux);
  //if((long)(micros() - last_micros) >= debouncing_time * 1000) {
  // data is coming so set data lines for input
  setInput();
  setStatusPort(H89_WRITE_OK);
  int delayCnt = 0;
  //digitalWrite(ISR_AWAKE, LOW);
  intr7E_cnt++;

  cmdDataPtr = 0;
  cmdFlag = 1;
//  setStatusPort(H89_READ_OK);
//
  // dataOut(t1++);
  // while(digitalRead(H89_READ_DATA) == 1)
  //   if(interruptCounter > 0)
  //     break;
  // setStatusPort(H89_WRITE_OK);
  //
 // digitalWrite(ISR_AWAKE, HIGH);
  portEXIT_CRITICAL_ISR(&Cmdmux);
}

// ************************************ Setup *************************************
void setup() {
  Serial.begin(115200);
  dataInTimePtr = 0;
  setUpOta();

  setPorts();

  pinMode(led1,OUTPUT);

  attachInterrupt(digitalPinToInterrupt(intr7C), intrHandle7C, FALLING);
  attachInterrupt(digitalPinToInterrupt(intr7E), intrHandle7E, FALLING);
  attachInterrupt(digitalPinToInterrupt(H89_READ_DATA), intrHandleRead7C, FALLING);
  cmdFlag = 0;
  setStatusPort(cmdFlag);
 
  Serial.println(version);
  Serial.printf("Status %d\n", cmdFlag);

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);

}

//************** LOOP ****************

void loop() {
  int sendCnt = 0, errCnt = 0 ;
  // termninal interaction code
  if(Serial.available() > 0) {
    String dataStr = Serial.readString();
    if(dataStr[0] == 'v')
      Serial.println(version);
    if(dataStr[0] == 'b'){
     Serial.println("Restart\n"); 
     ESP.restart();
      }
    if(dataStr[0] == 'r'){
      Serial.println("Resetting counters\n");
      last7C = intr7C_cnt = last7E = intr7E_cnt = 0;
      bitCtr = 0;
      t1 = 0;
      } 
    }

  // Check if all command bytes arrived
  if (cmdDataPtr >= CMD_LENGTH){
    for(sendCnt = 0; sendCnt < CMD_LENGTH; sendCnt++)
      dataOutBuf[dataOutBufPtr+sendCnt] = cmdData[dataOutBufPtr+sendCnt]+offset;
    offset++;
    dataOutBufLast = dataOutBufPtr + sendCnt  ;

    for(int i = 0; i < CMD_LENGTH; i++)
      Serial.printf("Cmd Byte %d\n", cmdData[i]);
    cmdDataPtr = 0;
   
    while(dataOutBufPtr < dataOutBufLast){
      if(dataOut(dataOutBuf[dataOutBufPtr]) == 0 ){
        Serial.printf("Sent %d\n",dataOutBuf[dataOutBufPtr]);
        dataOutBufPtr++;
        }
      else 
        errCnt++;  
      }
    dataOutBufPtr = 0 ;  
    dataOutBufLast = 0;
    if(errCnt > 0){
      Serial.printf("Data Out errors: %d\n", errCnt);
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