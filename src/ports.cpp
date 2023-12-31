// set ports for inut or output for parallel posrt operation
#include "settings.h"
// extern int dataInTime[100];
// extern int dataInTimePtr ;

extern int pins[];
extern int pinInOut;
extern int currentStatus;
extern int h89BytesToRead;
extern int h89ReadData ;
extern bool debugFlag;
extern unsigned long portChngCnt ;  // number of times port changed from input to output
extern unsigned long portChngTime;  // time required for port changes
extern uint32_t readChk ;         // # read attempts
extern uint32_t writeChk ;        // # write attempts

//******************************************

//******************** setOutput **********************
void setOutput(){
    int i ;
    unsigned long start;
    
    start = micros();
    portChngCnt++;
    for(i = 0; i < 8; i++){
        pinMode(pins[i], OUTPUT);
    }
    portChngTime += micros()-start;
}
//******************* setInput *****************
void setInput(){
    int i ;
    unsigned long start;

    start = micros();
    portChngCnt++;
    for(i = 0; i < 8; i++){
        pinMode(pins[i], INPUT_PULLUP);
    }
    portChngTime += micros()-start;
}
//******************* setPorts *****************
void setPorts(){

    pinMode(STATUS_BIT_0,OUTPUT);
    pinMode(STATUS_BIT_1, OUTPUT);
    pinMode(H89_READ_DATA, INPUT_PULLUP);
    pinMode(H89_WRITE_DATA,INPUT_PULLUP);
    pinMode(H89_CMD,INPUT_PULLUP);
    pinMode(DATA_IN_OE, OUTPUT);
    pinMode(DATA_OUT_OE, OUTPUT);

    digitalWrite(STATUS_BIT_0,LOW);
    digitalWrite(STATUS_BIT_1, LOW);
    digitalWrite(DATA_IN_OE, HIGH);
    digitalWrite(DATA_OUT_OE, HIGH);

}

//******************* setStatusPort *****************
void setStatusPort(byte status){
    // if (status != currentStatus)
    //     ets_printf("Status: Old %d New %d\n", currentStatus, status);
    currentStatus = status;
    digitalWrite(STATUS_BIT_0, status & 1);
    digitalWrite(STATUS_BIT_1, status & 2);
    if(debugFlag)
        ets_printf("Status: %d\n", status);
}


//********************* DataOut ******************
byte dataOut(byte data){
  //  if(h89ReadData == H89_OK_TO_READ)
   //Serial.printf("DataOut: %d\n", data);
    if(currentStatus == H89_READ_OK)
        {
        writeChk++;    
        return DATA_NOT_READ;
        }

    setStatusPort(ESP_BUSY );  
 
    if(pinInOut == DATA_IN){
        setOutput();
        pinInOut = DATA_OUT;
    }
    digitalWrite(DATA_OUT_OE, HIGH);        // Enable data to be set
    for(int i = 0; i < 8; i++){                  
        digitalWrite(pins[i], (data & 1)); 
        data = data >> 1;
    }
    digitalWrite(DATA_OUT_OE, LOW);     // Latch data
    // Tell the H89 to read byte
   // h89ReadData = H89_OK_TO_READ;
    setStatusPort(H89_READ_OK );  

    return DATA_SENT;
}
//****************************************
byte dataIn(){
    int data = 0;
    int i ;

    if(pinInOut == DATA_OUT){
        setInput();
        pinInOut = DATA_IN;
    }
    digitalWrite(DATA_IN_OE, LOW);
    for(i =0; i < 8; i++){
        data = data  + (digitalRead(pins[i]) << i );
    }
    digitalWrite(DATA_IN_OE, HIGH);
    //ets_printf("DI %x\n", data);
    return data;
}

