// set ports for inut or output for parallel posrt operation
#include "settings.h"

void setInput(){
    pinMode(32,INPUT_PULLUP);
    pinMode(33,INPUT_PULLUP);
    pinMode(25,INPUT_PULLUP);
    pinMode(26,INPUT_PULLUP);
    pinMode(27,INPUT_PULLUP);
    pinMode(14,INPUT_PULLUP);
    pinMode(12,INPUT_PULLUP);

}
void setOutput(){
    pinMode(32,OUTPUT);
    pinMode(33,OUTPUT);
    pinMode(25,OUTPUT);
    pinMode(26,OUTPUT);
    pinMode(27,OUTPUT);
    pinMode(14,OUTPUT);
    pinMode(12,OUTPUT);

}
void setOtherPort(){
    pinMode(11,INPUT_PULLUP);       // IO11, CMD, Flpy W L
    pinMode(10,INPUT_PULLUP);       // IO10, Flpy R L
    pinMode(intr7C,INPUT_PULLUP);       // IO0, Port FC
    //pinMode(15,INPUT_PULLUP); 
    pinMode(intr7D,INPUT_PULLUP);       // IO02, Port FD
    pinMode(intr7E,INPUT_PULLUP);       // IO15, Port FE

}
