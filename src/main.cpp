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
const char *menuStr = "Menu\n v: Prints version\n b: Reboots system\n r: Resets counters\n s: SD card test\n c: Clears NVM\n m: Prints menu\n l: Prints license\n w: Set up WiFi\n\n";
// Test LED blinkers
const byte led1 = ALIVE_LED;
unsigned long delayLed = 0;
volatile int t1 = 0;
volatile unsigned long timeOutStart = 0;

//************** H89 data flags and buffer
volatile int currentStatus = 0; // status value for H89 to read
// volatile int h89ReadData  = DATA_SENT;    // status value for H89 data actually read
volatile int h89BytesToRead = 0;
int offset = 1;
// Data In Buffer
extern byte dataInBuf[BUFFER_LEN];
extern int dataInPtr;
extern int dataInLast;  // pointer to valid data. No data iof dataInLast = dataInPtr
extern bool bufferFull; // flag to indicate buffer is full
// Data out bytes
// extern byte dataOutBuf[BUFFER_LEN];
// extern int dataOutBufPtr ;
// extern int dataOutBufLast ;
// Command control bytes
extern byte cmdData[CMD_LENGTH];
extern byte cmdDataPtr;
extern int8_t cmdFlag;
extern int8_t cmdLen;

//************* Interupt Counters
volatile uint32_t intr7C_cnt = 0;
volatile int intr7E_cnt = 0;
volatile uint32_t intr7CRead_cnt = 0;
int last7C = 0;
int last7E = 0;
int last7CRead = 0;

//************* Timing debug counters
volatile unsigned long cmdStart;     // H89 wrote to command line
volatile unsigned long cmdEnd;       // All command Bytes received
volatile unsigned long portChngCnt;  // number of times port changed from input to output
volatile unsigned long portChngTime; // time required for port changes
volatile unsigned long cmdLoopStart; // Command start execution
volatile unsigned long cmdLoopEnd;   // Command end execution
volatile uint32_t cmdWriteByte;      // bytes written for command
volatile uint32_t cmdReadByte;       // byte read for command
volatile uint32_t readChk;           // # read attempts
volatile uint32_t writeChk;          // # write attempts

//************* Interrupt flags
portMUX_TYPE Cmdmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE DataInmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE DataOutmux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// port pin definitions D0..D7
volatile int pins[] = {32, 33, 25, 26, 27, 14, 12, 13};
// Current data direction
volatile int pinInOut = DATA_OUT;

// ddebugging variables
// volatile int dataInTime[100];
// volatile int dataInTimePtr;
// volatile int bitCtr = 0;
// volatile int bits[100];
volatile bool debugFlag = false;

int sent[20], sentPtr;

// Debouncing parameters
// long debouncing_time = 1000; // Debouncing Time in Milliseconds
volatile unsigned long last_micros;
// volatile byte dataOutFlag = 1 ;

//***************** reset Counters
void resetCounters()
{
  Serial.printf("intr7C_cnt: %d, intr7E_cnt: %d, intr7CRead_cnt: %d\n", intr7C_cnt, intr7E_cnt, intr7CRead_cnt);
  Serial.println("Resetting counters\n");
  last7C = intr7C_cnt = last7E = intr7E_cnt = intr7CRead_cnt = last7CRead = 0;
  cmdFlag = 0;
  cmdLen = CMD_LENGTH;
  // bitCtr = 0;
  offset = 1;
  // Reset data in buffer pointers
  dataInPtr = 0;      // Ptr to next position to write data
  dataInLast = 0;     // pointer to valid data. No data iof dataInLast = dataInPtr
  bufferFull = false; // flag to indicate buffer is full
  setStatusPort(CMD_RDY);
  t1 = 0;
}

//******************* handle Menu *****************
void handleMenu()
{
  String dataStr = Serial.readString();
  switch (tolower(dataStr[0]))
  {
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
    resetCounters();
    break;
  case 's':
    Serial.println("SD Card test\n");
    sdTest();
    break;
  case 'c':
    Serial.println("Clear NVM\n");
    setConfig(true);
    break;
  case 'w':
    setupWifi();
    break;
  }
}

/********************************* Delay interrupt timer ***************************/
volatile int interruptCounter;
volatile int timeOutCounter = HOLD;
int totalInterruptCounter = 0;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  if (timeOutCounter >= 0)
  {
    if (timeOutCounter-- == 0)
    {
      ets_printf("Interrupt Timeout mS %lu\n", millis() - timeOutStart);
      ESP.restart();
    }
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

// ************************************ Interrupt Handler H89 Write Data *************************************
void IRAM_ATTR intrHandleWriteData()
{ // Data flag
  byte temp;
  portENTER_CRITICAL_ISR(&DataInmux);
  setStatusPort(ESP_BUSY);

  intr7C_cnt++;
  cmdWriteByte++;
  temp = dataIn();
  // ets_printf("Data %d, C Flag %d, C Ptr %d, D Last %d, D Ptr %d\n", temp, cmdFlag, cmdDataPtr, dataInLast, dataInPtr);
  if ((cmdFlag == 1) && (cmdDataPtr < cmdLen))
  {
    if (cmdDataPtr == 0)
      switch (temp)
      {
      case 0x08: // disk read/write
      case 0x0A:
      case 0x28:
      case 0x2A:
        cmdLen = 4;
        break;
      case 0x11: // list files with text
        cmdLen = CMD_LENGTH;
        break;
      case 0x12: // list files with file #
        cmdLen = 3;
        break;
      case 0x01: // debug
        cmdLen = 4;
        break;
      case 0x03: // last operation status
      case 0x10: // list files on micro SD card
      case 0x30: // upload file
      case 0x31: // download file
        cmdLen = 1;
        break;
      default:
        cmdLen = 1;
        ets_printf("Bad command %x\n", temp);
        break;
      }
    cmdData[cmdDataPtr++] = temp;
    // ets_printf("CMD Data %d\n", cmdData[cmdDataPtr-1]);
  }
  else
  {
    if (!bufferFull)
    {
      dataInBuf[dataInPtr++] = temp;
      // ets_printf("data: %d, dataPtr: %d\n",temp, dataInPtr-1);
      if (dataInPtr == BUFFER_LEN)
        dataInPtr = 0;
      if (dataInPtr == dataInLast)
        bufferFull = true;
    }
    else
      ets_printf("Buffer FULL - Data LOST %d\n", temp);
  }
  if (cmdDataPtr == cmdLen)
    cmdFlag = 0;
  // cmdEnd = micros();
  if (!bufferFull)
    setStatusPort(H89_WRITE_OK);
  else
    setStatusPort(ESP_BUSY);

  // ets_printf("cmdFlag %d, CmdDataPtr %d, DataInPtr %d, DataInLast %d, BufferFull %d\n", cmdFlag, cmdDataPtr,dataInPtr, dataInLast, bufferFull);
  //    ets_printf("dataOut Error: %d\n", errCnt);
  //  ataPtr, (long)micros()-last_micros);        // Safe to use in an interrupt routine
  // cmdFlag = 4;
  // digitalWrite(ISR_AWAKE, HIGH);
  portEXIT_CRITICAL_ISR(&DataInmux);
}

//******************************** get Data ************************************
// returns true on success and valid byte value
bool getData(byte &x)
{
  bool result = true;

  // if(offset == 1){
  //   Serial.printf("GD Bufferfull %d, dataInPtr %d, dataInLast %d\n", bufferFull, dataInPtr, dataInLast)  ;
  //   offset++;
  // }
  portENTER_CRITICAL_ISR(&DataInmux);
  // valid data if buffeFull false and dataInlast not equal to DataInPtr or if BufferFull is true
  if ((!bufferFull && dataInLast != dataInPtr) || bufferFull)
  {
    x = dataInBuf[dataInLast++];
    if (dataInLast == BUFFER_LEN)
      dataInLast = 0;
    bufferFull = false;
    // ets_printf("GD: %c\n", x);
  }
  else
  {
    readChk++;
    result = false;
  }

  // if(dataInLast == dataInPtr)
  //   bufferFull = true;
  portEXIT_CRITICAL_ISR(&DataInmux);
  return result;
}

// ************************************ Interrupt Handler H89 Read 7C *************************************
// H89 read port 7C
void IRAM_ATTR intrHandleReadData()
{ // Data flag
  portENTER_CRITICAL_ISR(&DataOutmux);
  // h89ReadData =  H89_GOT_DATA;
  intr7CRead_cnt++;
  // cmdReadByte++;
  // if (h89BytesToRead > 0)
  //   h89BytesToRead--;
  // if(h89BytesToRead == 0)
  //   setStatusPort(CMD_RDY);
  // else
  // ets_printf("H89 Read cnt: %d, h89 Bytes to Read %d\n", intr7CRead_cnt, h89BytesToRead);
  setStatusPort(ESP_BUSY);
  //}
  portEXIT_CRITICAL_ISR(&DataOutmux);
}
// ************************************ Interrupt Handler H89 Write 7E *************************************
// H89 wrote to Command Port
void IRAM_ATTR intrHandleCmd()
{ // Command flag
  portENTER_CRITICAL_ISR(&Cmdmux);
  // if((long)(micros() - last_micros) >= debouncing_time * 1000) {
  //  data is coming so set data lines for input
  setInput();
  setStatusPort(H89_WRITE_OK);
  intr7E_cnt++;
  cmdDataPtr = 0;
  if (cmdFlag == 1) // reset from H89
  {
    ets_printf("H89 Reboot Request\n");
    ESP.restart();
  }
  cmdFlag = 1;
  cmdStart = micros(); // Start timer for command execution
  readChk = writeChk = 0;
  cmdWriteByte = cmdReadByte = 0; // reset command byte counter
  portChngCnt = portChngTime = 0; // reset port change counter/timer

  portEXIT_CRITICAL_ISR(&Cmdmux);
}

// ************************************ Setup *************************************
void setup()
{
  // delay(4000); // startup delay to see if WiFi connects
  Serial.begin(115200);
  Serial.println(copyRightNoticeShort);
  // dataInTimePtr = 0;
  if (setupWifi())
  {
    Serial.println("Configuring Webserver ...");
    server = new AsyncWebServer(WEB_SERVER);
    configureWebServer();
  }
  if (!SD.begin())
  { // SD_CS,spi,80000000)){
    Serial.println("*******Card Mount Failed **********\n");
  }

  // H89 interface setup
  setPorts();
  setInput();
  pinInOut = DATA_IN; // data port pins flag

  pinMode(led1, OUTPUT); // Keep alive LED

  attachInterrupt(digitalPinToInterrupt(H89_WRITE_DATA), intrHandleWriteData, FALLING);
  attachInterrupt(digitalPinToInterrupt(H89_CMD), intrHandleCmd, FALLING);
  attachInterrupt(digitalPinToInterrupt(H89_READ_DATA), intrHandleReadData, FALLING);
  cmdFlag = CMD_RDY;
  setStatusPort(cmdFlag);

  Serial.println(version);
  Serial.printf("cmdFlag %d\n", cmdFlag);

  timer = timerBegin(0, 80, true);             // set interrupt interval to 1 micro sec. ESP32 base clock 80 MHz
  timerAttachInterrupt(timer, &onTimer, true); // set interrupt function to call
  timerAlarmWrite(timer, 1000000, true);       // set alarm for 1 sec
  timerAlarmEnable(timer);                     // enable interrupt function
  // debug stuff
  Serial.printf("Heap: Free %d, Min: %d, Size: %d, Alloc: %d\n", ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
  sentPtr = -1;
  Serial.printf(menuStr);
  digitalWrite(led1, HIGH);

  // crc Test
  // int j = 0;
  // byte *p;
  // for(j = 0; j < 10; j++)
  //   dataInBuf[j] = j;
  // p = dataInBuf;
  // Serial.printf("CRC Test %x\n", calcrc(p, 10))  ;
}

//************** LOOP ****************

void loop()
{

  if (shouldReboot)
  {
    rebootESP("Web Admin Initiated Reboot");
  }
  // termninal interaction code
  if (Serial.available() > 0)
    handleMenu();

  // Check if all command bytes arrived
  commands();
  // debug code
  // if(cmdFlag > 0){
  //   Serial.printf("Loop Command Byte: %X\n", cmdData[0]);
  //   for( int i = 1; i < cmdDataPtr; i++)
  //     Serial.println(cmdData[i]);
  // }
  // end debug code
  if (intr7C_cnt - last7C > 3)
  {
    Serial.printf("Interrupt 7C Write count = %d\n", intr7C_cnt);
    // Serial.printf("Cmd Ptr: %d Data Ptr: %d\n", cmdDataPtr, dataPtr);
    last7C = intr7C_cnt;
  }

  if (intr7E_cnt > last7E)
  {
    Serial.print("Interrupt 7E count = ");
    Serial.println(intr7E_cnt);
    last7E = intr7E_cnt;
  }
  if (intr7CRead_cnt > last7CRead)
  {
    Serial.print("Interrupt 7C Read count = ");
    Serial.println(intr7CRead_cnt);
    last7CRead = intr7CRead_cnt;
  }
  // Keep Alive LED blink code
  if (interruptCounter > 0)
  {
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);

    totalInterruptCounter++;
    if (totalInterruptCounter % 2 == 0)
      digitalWrite(led1, LOW);
    else
      digitalWrite(led1, HIGH);
  }
}

//******************** printDataBufPtr ************
void printDataBufPtr()
{

  Serial.printf("Data %x, C Flag %d, C Ptr %d, D Last %d, D Ptr %d\n", dataInBuf[dataInLast], cmdFlag, cmdDataPtr, dataInLast, dataInPtr);
}