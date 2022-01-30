#include "settings.h"

//portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile int intr7C_cnt = 0;
volatile int intr7D_cnt = 0;
volatile int  intr7E_cnt = 0;
int last7C = 0;
int last7D = 0;
int last7E = 0;


portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
 
void IRAM_ATTR intrHandle7C() {
  portENTER_CRITICAL_ISR(&mux);
  intr7C_cnt++;
  portEXIT_CRITICAL_ISR(&mux);
}
void IRAM_ATTR intrHandle7D() {
  portENTER_CRITICAL_ISR(&mux);
  intr7D_cnt++;
  portEXIT_CRITICAL_ISR(&mux);
}
void IRAM_ATTR intrHandle7E() {
  portENTER_CRITICAL_ISR(&mux);
  intr7E_cnt++;
  portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(led2,OUTPUT);
  pinMode(led1,OUTPUT);
  setOtherPort();
 
  attachInterrupt(digitalPinToInterrupt(intr7C), intrHandle7C, FALLING);
  attachInterrupt(digitalPinToInterrupt(intr7D), intrHandle7D, FALLING);
  attachInterrupt(digitalPinToInterrupt(intr7E), intrHandle7E, FALLING);
  setInput();
 
  Serial.println("Version 1.2d");

}

void loop() {
  // put your main code here, to run repeatedly:
  portENTER_CRITICAL(&mux);
  if(intr7C_cnt > last7C){
    Serial.printf("Interrupt 7C count = %d\n", intr7C_cnt);
    //Serial.println(intr7C_cnt);
    last7C = intr7C_cnt ;
  }
  if(intr7D_cnt > last7D){
    Serial.print("Interrupt 7D count = ");
    Serial.println(intr7D_cnt);
    last7D = intr7D_cnt ;
  }
  if(intr7E_cnt > last7E){
    Serial.print("Interrupt 7E count = ");
    Serial.println(intr7E_cnt);
    last7E = intr7E_cnt ;
  }
  portEXIT_CRITICAL(&mux);
  
  digitalWrite(led2,LOW);
  Serial.printf("off %d \n", led2);
  delay(1000);
  digitalWrite(led2,HIGH);
  Serial.println("on");
  delay(1000);
}