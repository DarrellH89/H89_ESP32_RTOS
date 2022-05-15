#include <arduino.h>
#include "settings.h"

  //************** H89 data flags 
extern int currentStatus  ;          // status value for H89 to read
// extern int h89ReadData  ;            // status value for H89 data actually read
// extern int h89BytesToRead ;

  //************** H89 data buffers
volatile byte dataInBuf[BUFFER_LEN] ;        // data received from H89
volatile int dataInPtr = 0;           // Ptr to next position to write data
volatile int dataInLast = 0;          // pointer to valid data. No data iof dataInLast = dataInPtr
volatile bool bufferFull = false;     // flag to indicate buffer is full
// Command control bytes
volatile byte cmdData[CMD_LENGTH];    // command data buffer
volatile byte cmdDataPtr = 0 ;        // Ptr to last valid data. If cmdFlag == 1, cmd data starts in position 0
volatile int8_t cmdFlag = 0;          // processing a command flag
volatile int8_t cmdLen = CMD_LENGTH;  // max length for a command

// Data out bytes
// volatile byte dataOutBuf[BUFFER_LEN];         // data to send to H89
// volatile int dataOutBufPtr = 0;         // Ptr to next byte to send to H89
// volatile int dataOutBufLast = 0;        // Last valid data in send buffer

// interrupt flags
extern portMUX_TYPE Cmdmux ;
extern portMUX_TYPE DataInmux ;
extern portMUX_TYPE DataOutmux;
extern portMUX_TYPE mux ;

// Timer interrupt counter
extern int totalInterruptCounter;

// Command functions
void debug();

//************* Timing debug counters
extern unsigned long cmdStart;
extern unsigned long cmdEnd;
extern unsigned long cmdLoopStart;
extern unsigned long cmdLoopEnd;
unsigned long bytesToSend =0;
volatile unsigned long cmdPortStart, cmdPortEnd;

//**************** commands
void commands(){
  unsigned long cmdGotStr =0 ;
  int t = 0;
  String temp = "";
  String fName = "";
    // Check if all command bytes arrived
  if (cmdDataPtr >= cmdLen){
    // load data to send back
    switch (cmdData[0]){
      case 1:
        cmdGotStr = micros();
        debug();
        break;  
      case 0x10:
        temp = listFiles(false);
        cmdGotStr = micros();
        sendH89String(temp);
        break;  
      case 0x30:
         fName = getH89FileName();
         Serial.print("Filename: ");
         Serial.println(fName);
         //Serial.printf("File Size: %d\n",getH89Int());
         if(!getH89File(fName))
          Serial.println("File upload failed");

         break;  
      default:
         break;
      }
    cmdFlag = 0;                  // done processing command, reset flag
    cmdLen = CMD_LENGTH; 
    setStatusPort(CMD_RDY)  ;
    Serial.printf("Debug timing. Cmd Start %lu, Cmd End %lu, Port Change %lu, Got String: %lu,\n Cmd Loop Start %lu, Cmd Loop End %lu, Bytes: %lu, USec/Byte %lu\n", 
      cmdStart,  cmdEnd-cmdStart, cmdPortEnd-cmdPortStart,  cmdGotStr-cmdStart, cmdLoopStart-cmdStart, cmdLoopEnd-cmdStart, bytesToSend, (cmdLoopEnd- cmdLoopStart)/bytesToSend);
  }  
// if( dataInPtr > t){
//   Serial.printf("DatainBufPtr: %d\n", dataInPtr);
//   t = dataInPtr;
// }
  // if(dataInPtr > 0){
  //   Serial.println("Data Received"); 
  //   for(int i = 0; i < dataInPtr; i++)
  //     Serial.printf("%d\n",dataInBuf[i]);
  //   portENTER_CRITICAL(&mux);
  //   dataInPtr = 0;  
  //   portEXIT_CRITICAL(&mux);
  //   }
  }

  //****************** debug() **************
  // Sends data directly using data in datOutBuf
  void debug(){
    int sendCnt = 0;
    long errCnt = 0 ;
    extern int offset ;
    int temp = 0;
    byte dataOutBuf[CMD_LENGTH];

    // add command bytes into data buffer plus offset value
    for(sendCnt = 0; sendCnt < cmdLen; sendCnt++)
      dataOutBuf[sendCnt] = cmdData[sendCnt]+offset;
    offset++;

    bytesToSend = 4 ;
    if(sendCnt == 4){        // send four bytes back to H89

      Serial.println("Start");
      //
      cmdPortStart = micros();
      setOutput();
      cmdPortEnd = micros();
      //
      cmdLoopStart = micros();
      while(temp < sendCnt){
        if(dataOut(dataOutBuf[temp]) == DATA_SENT){
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
  temp = 0;
  while(temp < 4){
    Serial.printf("Sent %d\n",dataOutBuf[temp++]);
    }

  if(errCnt > 0){
    Serial.printf("Data Ready Checks: %ld\n", errCnt);
    errCnt = 0;
    } 
  setStatusPort(CMD_RDY)  ;
  }

//************ Send H89 String ********************
// Sends data directly. Does not use dataOutBuf
void sendH89String(String sendIt){
  int strLen, strPtr = 0, retry = 0;

  strLen = sendIt.length()+1;
  Serial.printf("\n\nString length: %d\n", strLen);
  char strArray[strLen];
  strcpy(strArray, sendIt.c_str());
  strArray[strLen-2] = 0;             // should work with -1, not sure why it needs to be -2

  Serial.println(sendIt);
  strPtr = 0;           // shouldn't need this
  bytesToSend = strLen;

  cmdPortStart = micros();
  setOutput();
  cmdPortEnd = micros();
  cmdLoopStart = micros();


  while(strPtr < strLen){
    if(dataOut(strArray[strPtr]) == DATA_SENT){
      //Serial.printf("StrPtr: %d, Retry attempts: %d, strLen: %d, H89 Bytes %d\n", strPtr, retry, strLen, h89BytesToRead);
      strPtr++;
      retry = 0;
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

//*************************** getH89FileName() *********************
String getH89FileName(){
  int tooLong =0;
  byte data = 1;
  String result = "";
    
  tooLong = totalInterruptCounter ;
  while(data !=0){
    if(totalInterruptCounter - tooLong  > 10)   // break if too long - 10 seconds
       break;
    if(getData(data)){
      result += String(char(data));
    }   
  }


  return result;
}

//******************** getH89Int() ********************
int getH89Int(){
  int tooLong =0;
  int result = 0;         
  int cnt = 0 ;           // # of bytes
  byte data = 1;

    
  tooLong = totalInterruptCounter ;
  while(cnt < 2){
    if(totalInterruptCounter - tooLong  > 10)   // break if too long - 10 seconds
       break;
    if(getData(data))
      result += data * pow(256, cnt++);
    }
  return result ;
}

//******************** getDataTime() ********************
// x = value to return, time = milli seconds to wait
// returns time left
 int getDataTime( byte &x, int time){
  int start;

  x = 0;
  start = millis();
  while(!getData(x) && time > 0){
    if(millis()-start > 1)
      time--;
  }  
  //Serial.printf("GDT x %d\n", x);
  return time; 
}
//******************** sendDataTime() ********************
// x = value to return, time = milli seconds to wait
// returns time left
 int sendDataTime( byte x, int time){
  unsigned long start;
  
  start = millis();
  while(time > 0 ){
    if(dataOut(x) == DATA_SENT) 
      break;
    if(millis()-start > 1)
      time--;
  }  

  return time; 
}