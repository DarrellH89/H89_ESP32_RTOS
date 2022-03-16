// set ports for inut or output for parallel posrt operation
#include "settings.h"
extern int dataInTime[100];
extern int dataInTimePtr ;
// extern int bitCtr;
// extern int bits[];
extern int pins[];
extern int pinInOut;
extern int currentStatus;
//******************************************
// void setPorts(){
//     pinMode( SERIAL_IN,INPUT_PULLUP);
//     pinMode(SERIAL_OUT, OUTPUT);
//     pinMode(H89_CLOCK_ENABLE,OUTPUT);
//     pinMode(H89_CLOCK_IN ,OUTPUT);
//     //pinMode(H89_LOAD ,OUTPUT);
//     pinMode(DATA_RCLK,OUTPUT);
//     pinMode(DATA_SRCLK,OUTPUT);
//     pinMode(STATUS_BIT_0,OUTPUT);
//     pinMode(STATUS_BIT_1, OUTPUT);
//     pinMode(H89_READ_DATA, INPUT_PULLUP);
//     pinMode(intr7C,INPUT_PULLUP);
//     pinMode(intr7E,INPUT_PULLUP);
//     pinMode(ISR_AWAKE, OUTPUT);

//     digitalWrite(H89_CLOCK_ENABLE,HIGH);
//     digitalWrite(H89_CLOCK_IN ,HIGH);
//     //digitalWrite(H89_LOAD ,HIGH);
//     digitalWrite(DATA_RCLK,HIGH);
//     digitalWrite(DATA_SRCLK,HIGH);
//     digitalWrite(STATUS_BIT_0,LOW);
//     digitalWrite(STATUS_BIT_1, LOW);
//     digitalWrite(ISR_AWAKE, HIGH);
//     digitalWrite(SERIAL_OUT, HIGH);

// }
//******************************************
void setOutput(){
    int i ;
    for(i = 0; i < 8; i++){
        pinMode(pins[i], OUTPUT);
    }
}
void setInput(){
    int i ;
    for(i = 0; i < 8; i++){
        pinMode(pins[i], INPUT_PULLUP);
    }
}
void setPorts(){

    pinMode(STATUS_BIT_0,OUTPUT);
    pinMode(STATUS_BIT_1, OUTPUT);
    pinMode(H89_READ_DATA, INPUT_PULLUP);
    pinMode(intr7C,INPUT_PULLUP);
    pinMode(intr7E,INPUT_PULLUP);
    //pinMode(ISR_AWAKE, OUTPUT);
    pinMode(DATA_IN_OE, OUTPUT);
    pinMode(DATA_OUT_OE, OUTPUT);

    digitalWrite(STATUS_BIT_0,LOW);
    digitalWrite(STATUS_BIT_1, LOW);
    //digitalWrite(ISR_AWAKE, HIGH);
    digitalWrite(DATA_IN_OE, HIGH);
    digitalWrite(DATA_OUT_OE, HIGH);

}

//**********************
// void dataTo595( byte data){
//     int i ;
//     digitalWrite(STATUS_RCLK, LOW);
//     for(i = 0; i <8; i++)   {
//         digitalWrite(STATUS_SRCLK, LOW); 
//         digitalWrite(SERIAL_OUT, data & 0x01);
//         digitalWrite(STATUS_SRCLK, HIGH);
//         //Serial.println(data & 0x01);
//         data = data >>1;
//         }
//     digitalWrite(STATUS_RCLK, HIGH);
// }
//************************************
void setStatusPort(byte status){
    // int i ;
    // byte mask = 0x80;
    currentStatus = status;
    digitalWrite(STATUS_BIT_0, status & 1);
    digitalWrite(STATUS_BIT_1, status & 2);
    //ets_printf("Status: %d\n", status);
    // for(i = 0; i <8; i++)   {
    //     digitalWrite(STATUS_SRCLK, LOW); 
    //     digitalWrite(SERIAL_OUT, status & mask);
    //     digitalWrite(STATUS_SRCLK, HIGH);
    //     //Serial.println(data & 0x01);
    //     mask = mask >> 1;
    //     }
    // digitalWrite(STATUS_RCLK, HIGH);  
    // digitalWrite(SERIAL_OUT, HIGH) ;  
}
//***************************************
// void dataOut(byte data){
//     int i,j=0 ;
//     byte mask = 0x80;
//     //Serial.printf("mask: ");
//     digitalWrite(DATA_RCLK, LOW);
//     for(i = 0; i <8; i++)   {
//         digitalWrite(DATA_SRCLK, LOW); 
//         digitalWrite(SERIAL_OUT, data & mask);
//         //delayMicroseconds(2);
//         digitalWrite(DATA_SRCLK, HIGH);
//         // if(data & mask)
//         //     Serial.print("1");
//         //   else
//         //     Serial.print("0");
//       //  Serial.printf("%x ", mask);
//         mask = mask >> 1;
//         //Serial.println(data & 0x01);
//         //data = data >>1;
//         }
//     digitalWrite(DATA_RCLK, HIGH);   
//     digitalWrite(SERIAL_OUT, HIGH) ;  
//     //Serial.println();
// }
//***************************************
byte dataOut(byte out){
    if(currentStatus == H89_READ_OK)
        return DATA_NOT_READ;
    setStatusPort(ESP_BUSY );  
 
    if(pinInOut == DATA_IN){
        setOutput();
        pinInOut = DATA_OUT;
    }
    //Serial.printf("DataOut: %d\n", out);
    digitalWrite(DATA_OUT_OE, HIGH);        // Enable data to be set
    for(int i = 0; i < 8; i++){                  
        digitalWrite(pins[i], (out & 1)); 
    //    Serial.printf("%x\n",out );
        out = out >> 1;
    }
    digitalWrite(DATA_OUT_OE, LOW);     // Latch data
    // Tell the H89 to read byte
    setStatusPort(H89_READ_OK );  
    // can kill this later if status return isn't needed
    return DATA_SENT;
    // while (digitalRead(H89_READ_DATA) == HIGH)
    // {
    //     wait++;
    //     if(wait > MAX_READ_WAIT)
    //         break;                   // timeout error
    // }
    // setStatusPort(ESP_BUSY );  
    // if(wait > MAX_READ_WAIT)
    //     return 1;
    //   else  
    //     return 0;                           // no error
    
}
//****************************************
byte dataIn(){
    byte data = 0;
    byte i ;

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
// byte dataIn(){
//     //int last_micros = micros();
//     byte  data = 0, temp;
//     int i;
//     digitalWrite(ISR_AWAKE, HIGH);
//     digitalWrite(ISR_AWAKE, LOW);    
//     // digitalWrite(H89_CLOCK_IN, HIGH);
//     // digitalWrite(H89_CLOCK_ENABLE, LOW);
//     // data = shiftIn(SERIAL_IN, H89_CLOCK_IN,MSBFIRST);
//     // digitalWrite(H89_CLOCK_ENABLE, HIGH);
//     // return data;
//     // data = digitalRead(SERIAL_IN) << 7 ;

//     digitalWrite(H89_CLOCK_IN, HIGH);
//     digitalWrite(H89_CLOCK_ENABLE, LOW); 
//     for(i = 0; i < 8; ++i) {
//         digitalWrite(H89_CLOCK_IN, HIGH);
//         temp = digitalRead(SERIAL_IN) ;
//         data = (temp << (7 - i)) | data;
//         //data |= digitalRead(SERIAL_IN) << (7 - i);
//         //bits[bitCtr++] =temp;
//         digitalWrite(H89_CLOCK_IN, HIGH);  
//         digitalWrite(H89_CLOCK_IN, LOW);
//          //delayMicroseconds(5);
 
//     }
//     //ets_printf("%d\n",data);
//     digitalWrite(H89_CLOCK_ENABLE, HIGH);
//     //digitalWrite(H89_CLOCK_ENABLE, HIGH);
//     //dataInTime[dataInTimePtr++] = micros()-last_micros;
//     return data;
// }

