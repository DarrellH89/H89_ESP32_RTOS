// set ports for inut or output for parallel posrt operation
#include "settings.h"
// extern int dataInTime[100];
// extern int dataInTimePtr ;

extern int pins[];
extern int pinInOut;
extern int currentStatus;
extern int h89BytesToRead;
extern int h89ReadData ;
//******************************************

//******************** setOutput **********************
void setOutput(){
    int i ;
    for(i = 0; i < 8; i++){
        pinMode(pins[i], OUTPUT);
    }
}
//******************* setInput *****************
void setInput(){
    int i ;
    for(i = 0; i < 8; i++){
        pinMode(pins[i], INPUT_PULLUP);
    }
}
//******************* setPorts *****************
void setPorts(){

    pinMode(STATUS_BIT_0,OUTPUT);
    pinMode(STATUS_BIT_1, OUTPUT);
    pinMode(H89_READ_DATA, INPUT_PULLUP);
    pinMode(intr7C,INPUT_PULLUP);
    pinMode(intr7E,INPUT_PULLUP);
    pinMode(DATA_IN_OE, OUTPUT);
    pinMode(DATA_OUT_OE, OUTPUT);

    digitalWrite(STATUS_BIT_0,LOW);
    digitalWrite(STATUS_BIT_1, LOW);
    digitalWrite(DATA_IN_OE, HIGH);
    digitalWrite(DATA_OUT_OE, HIGH);

}

//******************* setStatusPort *****************
void setStatusPort(byte status){
    currentStatus = status;
    digitalWrite(STATUS_BIT_0, status & 1);
    digitalWrite(STATUS_BIT_1, status & 2);
    //ets_printf("Status: %d\n", status);
}


//********************* DataOut ******************
byte dataOut(byte data){
    if(h89ReadData == H89_OK_TO_READ)
        return DATA_NOT_READ;
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
    h89ReadData = H89_OK_TO_READ;
    setStatusPort(H89_READ_OK );  
 
    return DATA_SENT;
}
//****************************************
int dataIn(){
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
    return data;
}

