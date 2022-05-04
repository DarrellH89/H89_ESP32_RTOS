#include <arduino.h>
#include "settings.h"

  //************** H89 data flags and buffer
extern int currentStatus  ;          // status value for H89 to read
extern int h89ReadData  ;    // status value for H89 data actually read
extern int h89BytesToRead ;

volatile byte dataInBuf[512] ;
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

//************* Timing debug counters
extern unsigned long cmdStart;
extern unsigned long cmdEnd;
extern unsigned long cmdLoopStart;
extern unsigned long cmdLoopEnd;
int bytesToSend =0;
volatile unsigned long cmdPortStart, cmdPortEnd;

//**************** commands
void commands(){
  unsigned long cmdGotStr =0 ;
    // Check if all command bytes arrived
  if (cmdDataPtr >= cmdLen){
    // load data to send back
    switch(cmdData[0]){
      case 1:
        cmdGotStr = micros();
        debug();
        break;  
      case 0x10:
        String temp = listFiles(false);
        cmdGotStr = micros();
        sendH89String(temp);
        break;  
      }
    cmdFlag = 0;
    cmdLen = CMD_LENGTH; 
    Serial.printf("Debug timing. Cmd Start %lu, Cmd End %lu, Port Change %lu, Got String: %lu, Cmd Loop Start %lu, Cmd Loop End %lu, USec/Byte %lu\n", 
      cmdStart,  cmdEnd-cmdStart, cmdPortEnd-cmdPortStart,  cmdGotStr-cmdStart, cmdLoopStart-cmdStart, cmdLoopEnd-cmdStart, (cmdLoopEnd- cmdLoopStart)/bytesToSend);
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
    bytesToSend = h89BytesToRead;
    h89ReadData = DATA_SENT;          // Set to something other than H89_OK_TO_READ
    //Serial.printf(" Buffer Last %d, Buffer Ptr %d\n", dataOutBufLast, dataOutBufPtr);
    if(dataOutBufLast - dataOutBufPtr == 4){        // send four bytes back to H89
      int temp = dataOutBufPtr;
      Serial.println("Start");
      //
      cmdPortStart = micros();
      setOutput();
      cmdPortEnd = micros();
      //
      cmdLoopStart = micros();
      while(temp < dataOutBufLast){
        if(dataOut(dataOutBuf[temp]) == DATA_SENT){
          //Serial.printf("Data Ready Checks: %lu\n",errCnt);
          //sent[++sentPtr] = dataOutBuf[temp];
          temp++;
          }
        else 
          errCnt++;  
        }
      cmdLoopEnd = micros();
      Serial.println("End");
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
  dataOutBufPtr = 0 ;         // reset data buffer
  dataOutBufLast = 0;
  if(errCnt > 0){
    Serial.printf("Data Ready Checks: %ld\n", errCnt);
    errCnt = 0;
    } 
  setStatusPort(CMD_RDY)  ;
  }

//************ Send H89 String ********************
void sendH89String(String sendIt){
  int strLen, strPtr = 0, retry = 0;

  strLen = sendIt.length()+1;
  Serial.printf("\n\nString length: %d\n", strLen);
  char strArray[strLen];
  strcpy(strArray, sendIt.c_str());
  strArray[strLen-2] = 0;             // should work with -1, not sure why it needs to be -2

  // byte temp[strLen+1];
  for(int i = 0; i < strLen+1; i++)
     dataOutBuf[i] = strArray[i];
  Serial.println(sendIt);
  h89BytesToRead = strLen;
  // h89BytesToRead =50;
  //strArray[strLen] =0;
  h89ReadData = DATA_SENT;
  strPtr = 0;           // shouldn't need this
  bytesToSend = h89BytesToRead;
  //
  cmdPortStart = micros();
  setOutput();
  cmdPortEnd = micros();
  cmdLoopStart = micros();
  dataOutBufPtr = 0;
  // dataOut(dataOutBuf[dataOutBufPtr]);
  // while(dataOutBufPtr <= h89BytesToRead)
  //   if(micros()-cmdLoopStart > 60000)
  //     break;
  //
  while(strPtr < strLen){
    if(dataOut(strArray[strPtr]) == DATA_SENT){
      //Serial.printf("StrPtr: %d, Retry attempts: %d, strLen: %d, H89 Bytes %d\n", strPtr, retry, strLen, h89BytesToRead);
      strPtr++;
      //retry = 0;
    }
    else{
      retry++;  
    }
    if(retry > TIMEOUT)
      break;
    // Serial.printf("pos: %d Hex: %x Val: %c\n",strPtr,strArray[strPtr],strArray[strPtr]);
    // strPtr++;
    
  }
  cmdLoopEnd = micros();
  setStatusPort(CMD_RDY)  ;
  Serial.printf("Exit sendH89String %d, Retry Cnt: %d, Last Char %x\n", strPtr, retry, strArray[strPtr-1]);
}