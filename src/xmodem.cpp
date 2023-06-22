#include "settings.h"
#include "xmodem.h"

unsigned long timeStart = 0; 
unsigned long timeEnd =0;
extern int timeOutCounter ;
extern int currentStatus  ;          // status value for H89 to read
extern unsigned long timeOutStart ;
extern int h89BytesToRead ;
extern long bytesToSend;
extern unsigned long cmdLoopStart;
extern unsigned long cmdLoopEnd;
extern uint32_t  intr7CRead_cnt;
extern uint32_t intr7C_cnt;
extern int dataInPtr ;
extern int dataInLast ;
extern byte dataInBuf[BUFFER_LEN] ;
extern bool debugFlag;

//********** calc CRC ***********
int calcCRC(int crc, int data){
  int i;
  crc = crc ^ (int)(data) << 8 ;
  for(i = 0; i < 8; i++ )
    if (crc & 0x8000)
      crc = (crc << 1) ^ 0x1021 ;
    else
      crc = crc << 1 ;
  return crc;      
}

//************** get H89 File ******************
bool getH89File( String fname )  {
  int j, i;
  int k = 0;
  byte  first = 0;
  byte snum,scur, scomp, chk1, chk2, temp;
  int errors, errmax, transize ;
  uint16_t crc;
  bool ok = false;
  byte start ;
  byte buffer[BLOCK];
  int buffPtr = 0;
  static uint32_t lastMillis = 0;
  long retryLast;
  long retry = 0;

  errmax = errormax ;
  //time = 2000 ;                 /* initial timeout in ms for CRC mode */
  errors = 0;
//  rcvptr = rcvbuf;
  
  // Open SD card
  if(!SD.begin()){     //SD_CS,spi,80000000)){
        Serial.println("Card Mount Failed");
        return false;
    }
    uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return false;
    }
    // end SD card open

  Serial.printf("\nTrying to receive file ");
  fname = "/" + fname;
  Serial.println(fname);
  File file = SD.open(fname, "w");
  if(!file){
    const char * c = fname.c_str();
    Serial.printf("Failed to open file %s", c);
    return false;
  }
  errors = 0;
  start = byte('C') ;              /* Ask for CRC mode first */
  if(DEBUG)
    Serial.printf("Current Status %d, Sending: %x\n", currentStatus, start);
  h89BytesToRead = 1; 
  Serial.println("Sending 'C'");
  if(sendDataTime(start,500)==0){
    if(DEBUG)
      Serial.println("Start timeout\n");
    return false;
  }
  if(DEBUG)
    Serial.printf("Currentstatus: %d\n", currentStatus);
  timeOutCounter = 10;               // timeout counter set to 10 sec, Error at 8 sec. It it reaches 0m the ESP32 reboots
  timeOutStart = millis();

  while(h89BytesToRead > 0){      // wait until H89 reads data
    errors++;
    if(currentStatus == ESP_BUSY)
      break;
    if(DEBUG && errors % 500 == 0)  
      Serial.printf("Error Cnt %d\n", errors);
  }  
  timeOutCounter = HOLD;   
  if(DEBUG) {  
    Serial.printf("Currentstatus: %d, Data Sent Checks %d\n", currentStatus, errors);
    printDataBufPtr();
  }

  snum = 0 ;
  errors = 0 ;
  transize = 128 ;        // data packet size
  bytesToSend =  intr7C_cnt;      // used for timing analysis
  cmdLoopStart = micros();
  do  {               // get data loop
    setStatusPort(H89_WRITE_OK);
    ok = FALSE ;
    first = 0;
    retry = 0;
    retryLast = retry;
    while (!(( first == SOH)||( first == EOT)) ){
      if(!getData( first) )
        retry++;
     // delay(10)  ;
      if(DEBUG && retry % 30000 == 0 && k != dataInPtr && retry > retryLast){
        retryLast = retry;
        Serial.println("***************");
        for (j = k; j <  dataInPtr; j++)
          if(dataInBuf[j]<20 || dataInBuf[j]> 128)
            Serial.printf("\n%x\n", dataInBuf[j]);
          else 
            Serial.print(char(dataInBuf[j]));
        k = j;    
        // Serial.print("Do Loop: ");
        // printDataBufPtr();  
        Serial.printf("\n-------------- SOH/EOT Retry: %d, Last: %d, Ptr: %d \n", retry, dataInLast, dataInPtr);
      }
      if(retry > TIMEOUT_CNT*3)
          break;
      }
    if(DEBUG)  
      Serial.printf("retry: %lu, first %x\n", retry, first)  ;
    if( first == SOH ) {
      getDataTime(scur, TIMEOUT);          /*  real sector number  */
      getDataTime(scomp, TIMEOUT);         /*  inverse of above */
      if(DEBUG)
        Serial.printf("scur: %d, scomp %d\n", scur, scomp);
      if( (scur + scomp)  == 255) { /*<-- becomes this #*/
        if( (scur == (0xff & (snum + 1))) ) {       /* Good sector count */
          crc = 0 ;             /* CRC mode */
          for( j = 0; j < transize; j++) {      // get data packet
            timeOutCounter = 10;               // timeout counter set to 10 sec
            timeOutStart = millis();
            while(!getData(buffer[buffPtr]) );
            timeOutCounter = HOLD; 
            crc = calcCRC(crc,(int)(buffer[buffPtr]));
            buffPtr++;        
          }
          if(DEBUG)
            Serial.printf("Calculated CRC %x\n", crc); 
          if((getDataTime( chk1, TIMEOUT ) == 0) || (getDataTime( chk2, TIMEOUT )==0)) {    /* hi CRC byte */ /* lo CRC byte */
            Serial.println("CRC Timeout")   ;
            errors ++;
            //return false;
          }
          if(DEBUG)  
            Serial.printf("chk1 %x, chk2 %x\n", chk1, chk2);
          if ( ((chk1 << 8) + chk2 ) == crc ){
            ok = TRUE ;
            if(DEBUG)  
              Serial.println("CRC ok");
          }
          else
            if(DEBUG)
              Serial.printf("CRC bad %x\n",uint16_t(chk1 << 8) + chk2 )  ;
          if ( ok ) {
            snum = snum + 1;
            if(DEBUG)  
              Serial.printf("Received sector %d\n",snum );
            errmax = errors + errormax ;      /* Good block- Reset error maximum */
            if( buffPtr >= BLOCK) {     /* check for full buffer */
              file.write( buffer, BLOCK);
              buffPtr = 0;
              }
            }
          else
            Serial.printf("\nError # %d - Bad Block %d\n", errors+1, snum );
          }
          else      // deal with bad sector count
            if (scur == (0xff & snum )) {                       /* wait until done */
              while ( getDataTime(temp, 100) != 0) ;    // read data until timeout
              Serial.printf("\nReceived duplicate sector %d\n", scur);
              start = ACK ;   /* trick error handler */
              }
            else {     /* fatal error */
              Serial.printf("\nError # %d - Synchronization error. got: %d, epected: %d\n", errors+1, scur, snum );
              start = EOT ;
              errors += errormax ;
              }
      }
      else
        Serial.printf("\nError # %d - Sector number error. scur %d, not scur: %d\n", errors+1, scur,  scomp );
    }
    if(first != EOT){
         if ( ok )  {  /* got a good sector and finished all processing so - */
            while(dataOut( ACK ) != DATA_SENT);           /* send request for next sector */
            if(DEBUG)  
              Serial.println("ACK Sent");
          }
          else {                       /* some error occurred! */
            errors++ ;
            Serial.printf("Looking for SOH or EOT, got: %d\n", first);
            //while (getDataTime(first, 100) != 0) ;
            while(dataOut( start ) != DATA_SENT);
            start = NAK ;
            }
        retry = 0;    
        if(getDataTime(first, TIMEOUT) == 0) 
          errors++;
        if(DEBUG)    
          Serial.printf("first: %d\n", first)  ;
    }
    } while( (first != EOT)&&(errors <= errmax));     // Exit data packet Loop
  cmdLoopEnd = micros() - cmdLoopStart;
  bytesToSend = intr7C_cnt - bytesToSend;     // number of bytes received
  if(DEBUG)  
    Serial.printf("Exited Do Loop first: %x, errors: %d\n", first, errors);
  if(( first == EOT )&&( errors < errmax ))
      {
      while (dataOut( ACK )!= DATA_SENT);
      Serial.printf("\nTransfer complete\n") ;
      }
    else
      printf("\nAborting");
  if( buffPtr > 0)    /* clear buffer */
      {
      while( buffPtr%BLKSIZE != 0 )
          buffer[buffPtr++] = 0;
      file.write( buffer, buffPtr);
      }
  file.close();    
  return true;
}


/********************* Send H89 File **********************/
/* Send a file XMODEM
*/

 bool sendH89File( String fname)
  {
  int j, k , time;
  bool result = true;
  byte first, snum;
  int transize, flagEOT ;
  uint32_t errors, nbytes, errmax, readerr, fileSize;
  uint16_t crc ;
 //bool ok = false;
  byte start ;
  byte buffer[BLOCK];
  int buffPtr = 0;
  static uint32_t lastMillis = 0;
  //long retry = 0;
  //long retryLast ;
  bool ok = true;
  long a, b ;

  errmax = errormax ;
  errors = 0;
  setStatusPort( H89_WRITE_OK);
  // Open SD card
  if(!SD.begin()){     //SD_CS,spi,80000000)){
        Serial.println("Card Mount Failed");
        ok = false;
    }
    uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        ok = false;
    }
    // end SD card open


  fname = "/" + fname;
  Serial.println(fname);
  File file = SD.open(fname, "r");
  if(!file){
    const char * c = fname.c_str();
    Serial.printf("Failed to open file %s", c);
    ok =  false;
  }
  fileSize = file.size();
  Serial.printf("\nTrying to send file, %d bytes \n", fileSize);
  if(!ok){
    while(dataOut( EOT ) != DATA_SENT);
    return false;
  }

  transize = 128 ;    /*  block transmission size */
  errmax = errormax;
  snum = 1 ;
  errors = 0;
  flagEOT = 0 ;
  file.seek(0, SeekSet);
  nbytes = file.read( buffer, BLOCK );    /* get the first block of data */
  bytesToSend = nbytes;
  if ( nbytes < BLOCK )
      flagEOT += 1 ;       /* flag last block read */
  if (nbytes == 0 )
      {
      printf( "Error! Empty File\n") ;
      goto ESCAPE ;
      }
  if(DEBUG)
    Serial.printf("Bytes Read: %d, FlagEOT %d\n", nbytes, flagEOT)    ;
  // Transfer loop
  timeOutCounter = 10;
  timeOutStart = millis();
  k = dataInLast ;        // debug data pointer
  do   {       /* check for initial 'C'' to start transfer */
    if ( getDataTime(first, TIMEOUT) == 0 ){
      errors ++;
      if( DEBUG && errors % 5 == 0)
        Serial.printf("Initial loop errors: %d, first: %x\n", errors, first)    ;
      }
    if (first != byte('C') ){
      errors ++ ;            /* increment error counter */
      if( DEBUG && errors % 5 == 0)
        printf( "Error! Timeout # %d.  Recieved %x\n", errors, first );
      }
  } while ( ((first != byte('C'))&&(first != NAK))&&(errors < errmax )) ;
  timeOutCounter = HOLD;

  if ( (first == byte('C')) )            /* okay to start transfer */
    {
    //if(DEBUG)
      Serial.println("Start sending file, got 'C'");
    cmdLoopStart = micros();
    do  {           // Transfer loop
      errors  = 0 ;        /* reset counter */
      if(DEBUG)
        Serial.printf("Sending sector # %d\n",  snum );
      timeOutCounter = 10 ;               // set timeout for 10 sec
      timeOutStart = millis();      
      // Send Header
      while(dataOut( SOH ) != DATA_SENT);
      /*
      if(sendDataTime(snum, 500)== 0 && DEBUG)        // alternative send method
        Serial.printf("snum send error %d\n", snum);
        */
      while(dataOut( snum ) != DATA_SENT);
     // while(dataOut( ~snum ) != DATA_SENT);     /* 1's complement */
      if(sendDataTime(~snum,1500) == 0) {
        Serial.println("~snum error");
      };
      timeOutCounter = HOLD;
      // Send data block
      crc = 0 ;
      for ( j = 0; j < transize ; j++ )   /* send one block */
        {
        if(DEBUG && snum == 1)
            Serial.printf("bPtr %d data %x\n", buffPtr, buffer[buffPtr])  ;
        while( (sendDataTime( buffer[buffPtr], 500) == 0) &&(errors < errmax) )
          errors++;
        if( errors > errmax)  
          goto ESCAPE;
        crc = calcCRC(crc,(int)(buffer[buffPtr++]));
        }
      if(DEBUG)
        Serial.printf("CRC: %x MSB %x LSB %x\n", crc,(crc >> 8)&0x00ff, crc & 0x00ff)  ;
      timeOutCounter = 2;  // 2 seconds
      timeOutStart = millis();
      debugFlag = false;    // debug flag for setStatusPort
      time = 0;  
      while( sendDataTime( (crc >> 8)&0x00ff, 1000 ) == 0)
        time++;
      while( sendDataTime( crc & 0x00ff, 1000 ) == 0)    
        time++;
      if(DEBUG)
        Serial.printf("CRC retries %d, bytes to read %d\n", time, h89BytesToRead)  ;
      readerr = 0;  
      //debugFlag = true;
      while(currentStatus != ESP_BUSY) {
        readerr++;
        // if(readerr == 1)
        //   Serial.printf("CurrentStatus: %d\n", currentStatus);
        delay(20);
        if(DEBUG && readerr % 500 == 0)  
          Serial.printf("Send Cnt: %d, Wait Cnt %d\n", time, readerr);  
        if(readerr > TIMEOUT*5)  {
          Serial.printf("CRC timeout Send Cnt: %d, Wait Cnt %d\n", time, readerr);
          errors = errormax;
          debugFlag = false;
          goto ESCAPE;
        }
      //debugFlag = false;  
      };        // wait for H89 to read last byte, busy set by intr 7C read routine
      //Serial.printf("readerr: %d\n", readerr)  ;

      timeOutCounter = HOLD;
      setStatusPort(H89_WRITE_OK);
      debugFlag = false;  
      do    {      /* check for ACK timeout for block transfer */
        a = millis();
        time = getDataTime( first, TIMEOUT);
        if (time == 0 ){
          errors ++ ;            /* increment error counter */
          Serial.printf(" ACK timeout %d ms\n", millis()-a);
        }
        // Debug print data in buffer  
        // if(DEBUG &&  k != dataInPtr){
        //   Serial.println("***************");
        //   for (j = k; j <  dataInPtr; j++)
        //     if(dataInBuf[j]<20 || dataInBuf[j]> 128)
        //       Serial.printf("\n%x\n", dataInBuf[j]);
        //     else 
        //       Serial.print(char(dataInBuf[j]));
        //   k = j;    
        //   Serial.printf("\n-------------- Errors: %d, Last: %d, Ptr: %d \n", errors, dataInLast, dataInPtr);
        //   }
        }  
        while ( (time == 0)&&(errors < errormax) ) ;
        if(DEBUG && errors == errormax)
          Serial.println("ACK Timeout error");
        if ( first == ACK )
          {
          snum += 1 ;    /* increment sector counter */
          if ( flagEOT > 0 && buffPtr >= nbytes)         /* already read last block */
            nbytes = 0 ;
          else      /* need to fill buffer */
            if(buffPtr >= nbytes){
              nbytes = file.read( buffer, BLOCK );    /* get the next block of data */
              bytesToSend += nbytes;
              buffPtr = 0;
              if ( nbytes < BLOCK )
                  flagEOT += 1 ;       /* flag last block read */
            }  

          }    
          else  if(errors<errormax)       /* Error, garbled ACK */
            {
            errors++ ;
            if (first == NAK)
                printf("Error! Bad Sector # %d, got %x\n", snum, first ) ;
              else
                printf("Error! Garbled ACK, Sector # %d, got %x\n", snum, first ) ;
            } 
          if(DEBUG)
            Serial.printf("Bytes Read: %d, FlagEOT %d, BuffPtr: %d\n", nbytes, flagEOT, buffPtr)    ;  
        } while ( (nbytes > 0)&&(errors < errormax ) ) ;
    }       // end of send file
  else
    {
    Serial.println("Failure to start file transfer") ;
    errors = errormax;
    }
  // cleanup effort
  cmdLoopEnd = micros() - cmdLoopStart;
  ESCAPE:
  if (DEBUG)
    Serial.printf("Exited Do Send Loop. Errors %d\n", errors);
  if (errors < errormax ){
    errors = 0;       // reset error count
    printf("Transfer complete. Sending EOT\n") ;  
    if (sendDataTime( EOT, 500) == 0)  
      Serial.println("EOT send timeout") ;
    if ( (getDataTime(first, 500)) > 0 )   
      {
      if( first != ACK)   
        printf("\nError! EOT not ACKed.  Recieved %x\n", first );
      // do {
      //     if ( (getDataTime(first, 500)) == 0 )
      //         {
      //         errors += 1 ;
      //         printf("\nError! EOT not ACKed.  Recieved %x\n", first );
      //         sendDataTime( EOT, 500) ;                    /* resend on error */
      //         }
      //     } while ( (first != ACK)&&( errors < 4 )) ;
      }
    }
    else {
      printf("Aborting\n");
      result = false;
    }
 
  file.close()  ;
  timeOutCounter = HOLD;
  return result;
  }


//*************** calCrc ***************
uint16_t calcrc(byte *ptr, int count)
{
    uint16_t  crc;
    char i;
    crc = 0;
    while (--count >= 0)
    {
        crc = crc ^ (int) *ptr++ << 8;
        i = 8;
        do
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while(--i);
    }
    return (crc);
}
