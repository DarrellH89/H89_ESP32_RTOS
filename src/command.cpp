#include <arduino.h>
#include "settings.h"

  //************** H89 data flags and buffer
extern int currentStatus  ;          // status value for H89 to read
extern int h89ReadData  ;    // status value for H89 data actually read
extern int h89BytesToRead ;

volatile byte dataInBuf[256] ;
volatile int dataInPtr = 0;
// Command control bytes
volatile byte cmdData[10];
volatile byte cmdDataPtr = 0 ;
volatile int8_t cmdFlag = 0;
volatile int8_t cmdLen = CMD_LENGTH;

// Data out bytes
volatile byte dataOutBuf[1024];
volatile int dataOutBufPtr = 0;
volatile int dataOutBufLast = 0;

// interrupt flags
extern portMUX_TYPE Cmdmux ;
extern portMUX_TYPE DataInmux ;
extern portMUX_TYPE DataOutmux;
extern portMUX_TYPE mux ;

// Command functions
void debug();

//**************** commands
void commands(){

    // Check if all command bytes arrived
  if (cmdDataPtr >= cmdLen){
    // load data to send back
    switch(cmdData[0]){
      case 1:
        debug();
        break;  
      }
  }  


  if(dataInPtr > 0){
    Serial.println("Data Received"); 
    for(int i = 0; i < dataInPtr; i++)
      Serial.printf("%d\n",dataInBuf[i]);
    portENTER_CRITICAL(&DataInmux);
    dataInPtr = 0;  
    portEXIT_CRITICAL(&DataInmux);
    }
  }

  //****************** debug() **************
  void debug(){
    int sendCnt = 0;
    long errCnt = 0 ;
    extern int offset ;

    // add command bytes into data buffer plus offset value
    for(sendCnt = 0; sendCnt < cmdLen; sendCnt++)
      dataOutBuf[dataOutBufPtr+sendCnt] = cmdData[sendCnt]+offset;
    offset++;
    dataOutBufLast = dataOutBufPtr + sendCnt  ;
    // Send 4 bytes of buffer data
    h89BytesToRead = 4;               // set value for H89 bytes to send 
    h89ReadData = DATA_SENT;
    Serial.printf(" Buffer Last %d, Buffer Ptr %d\n", dataOutBufLast, dataOutBufPtr);
    if(dataOutBufLast - dataOutBufPtr == 4){        // send four bytes back to H89
      int temp = dataOutBufPtr;
      while(temp < dataOutBufLast){
        if(dataOut(dataOutBuf[temp]) == 0 ){
          //sent[++sentPtr] = dataOutBuf[temp];
          temp++;
          }
        else 
          errCnt++;  
        }
    
    // print cmd bytes received
    for(int i = 0; i < cmdLen; i++){
      Serial.printf("Cmd Byte %d\n", cmdData[i]);
      cmdData[i] = 0;                 // reset command buffer
    }
    cmdDataPtr = 0;
  }
  // print data sent to H89
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