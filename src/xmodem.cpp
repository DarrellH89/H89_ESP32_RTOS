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

bool getH89File( String fname )  {
  int j, i ;
  byte  first = 0;
  byte snum,scur, scomp, chk1, chk2, temp;
  int errors, errmax ;
  int transize ;
  uint16_t crc;
  bool ok = false;
  byte start ;
  byte buffer[BLOCK];
  int buffPtr = 0;
  static uint32_t lastMillis = 0;
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
    Serial.printf("Current Status %d, Sending: %d\n", currentStatus, start);
  h89BytesToRead = 1; 
  if(DEBUG && sendDataTime(start,500)==0){
    Serial.println("Start timeout\n");
    return false;
  }
  if(DEBUG)
    Serial.printf("Currentstatus: %d\n", currentStatus);
  timeOutCounter = 10;               // timeout counter set to 10 sec, Error at 8 sec. It it reaches 0m the ESP32 reboots
  timeOutStart = millis();

  while(h89BytesToRead > 0){
    errors++;
    if(currentStatus == ESP_BUSY)
      break;
    if(DEBUG && errors % 500 == 0)  
      Serial.printf("Error Cnt %d\n", errors);
  }  
  timeOutCounter = HOLD;   
  if(DEBUG) {  
    Serial.printf("Currentstatus: %d, Data Sent Checks%d\n", currentStatus, errors);
    printDataBufPtr();
  }

  snum = 0 ;
  errors = 0 ;
  transize = 128 ;
  bytesToSend =  intr7C_cnt;
  cmdLoopStart = micros();
  do  {
    setStatusPort(H89_WRITE_OK);
    ok = FALSE ;
    first = 0;
    retry = 0;
    while (!(( first == SOH)||( first == EOT)) ){
      if(!getData( first) )
        retry++;
      if(DEBUG && retry % 200 == 0){
        Serial.print("Do Loop: ");
        printDataBufPtr();  
      }
      if(retry > 10000)
          break;
      }
    if(DEBUG)  
      Serial.printf("retry: %u, first %d\n", retry, first)  ;
    if( first == SOH ) {
//      time = 100;  
      getDataTime(scur, TIMEOUT);          /*  real sector number  */
      getDataTime(scomp, TIMEOUT);         /*  inverse of above */
      if(DEBUG)
        Serial.printf("scur: %d, scomp %d\n", scur, scomp);
      if( (scur + scomp)  == 255) { /*<-- becomes this #*/
        if( (scur == (0xff & (snum + 1))) ) {       /* Good sector count */
          crc = 0 ;             /* CRC mode */
          for( j = 0; j < transize; j++) {
            timeOutCounter = 10;               // timeout counter set to 10 sec
            while(!getData(buffer[buffPtr]) );
            timeOutCounter = HOLD; 
            crc = crc ^ (int)(buffer[buffPtr]) << 8 ;
            for(i = 0; i < 8; i++ )
              if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021 ;
              else
                crc = crc << 1 ;
            buffPtr++;        
          }
          if(DEBUG)
            Serial.printf("Calculated CRC %x\n", crc); 
          if((getDataTime( chk1, TIMEOUT ) == 0) || (getDataTime( chk2, TIMEOUT )==0)) {    /* hi CRC byte */ /* lo CRC byte */
            Serial.println("CRC Timeout")   ;
            return false;
          }
          if(DEBUG)  
            Serial.printf("chk1 %d, chk2 %d\n", chk1, chk2);
          if ( ((chk1 << 8) + chk2 ) == crc ){
            ok = TRUE ;
            if(DEBUG)  
              Serial.println("CRC ok");
          }
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
            Serial.printf("\nError # %d - Bad Block\n", errors+1 );
          }
          else      // deal with bad sector count
            if (scur == (0xff & snum )) {                       /* wait until done */
              while ( getDataTime(temp, 100) != 0) ;    // read data until timeout
              Serial.printf("\nReceived duplicate sector %d\n", scur);
              start = ACK ;   /* trick error handler */
              }
            else {     /* fatal error */
              Serial.printf("\nError # %d - Synchronization error.\n", errors+1 );
              start = EOT ;
              errors += errormax ;
              }
      }
      else
        Serial.printf("\nError # %d - Sector number error. Got: %d, Expected: %d\n", errors+1, scur,  snum+1 );
    }
    if(first != EOT){
         if ( ok )  {  /* got a good sector and finished all processing so - */
            while(dataOut( ACK ) != DATA_SENT);           /* send request for next sector */
            if(DEBUG)  
              Serial.println("ACK Sent");
        }
          else {                       /* some error occurred! */
            errors++ ;
            Serial.println("Not OK");
            while (getDataTime(first, 100) != 0) ;
            while(dataOut( start ) != DATA_SENT);
            start = NAK ;
            }
        retry = 0;    
        if(getDataTime(first, TIMEOUT) == 0) 
          errors++;
        if(DEBUG)    
          Serial.printf("first: %d\n", first)  ;
    }
    } while( (first != EOT)&&(errors <= errmax));
    // Exit do Loop
  cmdLoopEnd = micros();
  bytesToSend = intr7C_cnt - bytesToSend;
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


// /*********************  SendFile ***************************/

//  SendFile()
//   {
//   char filename[20];
//   int fp, j;
//   char c ;

//   printf("\n\nFilename.Ext ? ");

//   j = 0;
//   do                   /* Get line in Char mode  for CP/M & HDOS */
//      if ( ((c = GetCon()) != 0)&&(c != CR) )
//          {
//          filename[ j++ ] = c ;
//          putchar( c ) ;
//          }
//      while ( (c != CR)&&( j < 20 ))  ;
//   filename[ j ] = 0 ;                    /* terminate the string */

//   if (strlen(filename) > 0)
//       {
//       fp = fopen( filename, "rb");
//       if( fp == 0)
//           printf("\nFile open  error!!  Can't open %s %d\n", filename, fp);
//         else
//           {
//           SendFX( fp );
//           fclose( fp );
//           }
//       }
//   }

// /********************* Send FX **********************/
// /* Send a file XMODEM
// */

// SendFX( fp )
//   int fp;
//   {
//   static int j, i ;
//   static int first, snum, errors, chksum, nbytes, transize, flagEOT ;
//   static int crc ;
//   static int mode ;    /* 0 = Checksum, 1 = CRC, 2 = Ymodem */

//   int fpo ;

//   snum = 1 ;
//   mode = 0 ;
//   transize = 128 ;    /*  block transmission size */
//   errors = 0;
//   flagEOT = 0 ;
//   rcvptr = rcvbuf;

//   nbytes = read( fp, rcvbuf, BLOCK );    /* get the first block of data */
// /*
// fpo = fopen( "dummy","wb") ;
// write( fpo, rcvbuf, BLOCK );
// fclose( fpo ) ;
// fpo = fopen( "dummy1","wb") ;
// printf("rcvptr %x, rcvbuf %x\n", rcvptr, rcvbuf ) ;
// */
//   if ( nbytes < BLOCK )
//       flagEOT += 1 ;       /* flag last block read */
//   if (nbytes == 0 )
//       {
//       printf( "Error! Empty File\n") ;
//       goto ESCAPE ;
//       }
//   printf("Trying to send file\n");
//   do          /* check for initial NAK to start transfer */
//       if ( (first = getDataTime(60)) != NAK )
//         if (first == 'C' )
//             {
//             mode = 1 ;
//             printf( "CRC requested\n" );
//             }
//           else
//             {
//             errors += 1 ;            /* increment error counter */
//             printf( "Error! Timeout # %d.  Recieved %x\n", errors, first );
//             if (GetCon() == CAN)
//                 errors += errormax ;    /* Keyboard abort */
//             }
//       while ( ((first != 'C')&&(first != NAK))&&(errors < errormax )) ;

//   if ( (first == NAK)||(first == 'C') )            /* okay to start transfer */
//       {
//       errors  = 0 ;        /* reset counter */
//       do  {
//           printf("%cSending sector # %d", CR, snum );
//           if (GetCon() == CAN)
//               errors += errormax ;    /* Keyboard abort */
//           Send( SOH );
//           Send( snum ) ;
//           Send( ~snum ) ;   /* 1's complement */
//           chksum = 0 ;
//           crc = 0 ;
//           for ( j = 0; j < transize ; j++ )   /* send one block */
//               {
//               Send( *(rcvptr+j) );
//               switch( mode )
//                   {
//                   case 0: chksum += *(rcvptr+j) ;
//                           break ;
//                   case 1: crc = crc ^ (int) *(rcvptr+j)<<8 ;
//                           for(i = 0; i < 8; i++ )
//                               if (crc &0x8000)
//                                   crc = crc << 1 ^ 0x1021 ;
//                                 else
//                                   crc = crc << 1 ;
//                           break ;
//                   }
//               }
//           switch ( mode )
//               {
//               case 0: Send( chksum ) ;
//                       break ;
//               case 1: Send( (crc >> 8)&0x00ff ) ;
//                       Send( crc & 0x00ff )    ;
//                       break ;
//               }
//           do          /* check for ACK timeout for block transfer */
//               if ( (first = getDataTime(10)) == timeout )
//                   errors += 1 ;            /* increment error counter */
//               while ( (first == timeout)&&(errors < errormax) ) ;
//           if ( first == ACK )
//               {
//               snum += 1 ;    /* increment sector counter */
//               rcvptr += transize ;    /* update buffer pointer */
//               if ( ((unsigned) rcvptr - (unsigned) rcvbuf) >= nbytes )
//                   if ( flagEOT > 0 )         /* already read last block */
//                       nbytes = 0 ;
//                     else
//                       {                      /* need to fill buffer */
//                       nbytes = read( fp, rcvbuf, BLOCK );
//                       printf("\nReading %d bytes!\n", nbytes);
//                       rcvptr = rcvbuf ;
//                       if ( nbytes < BLOCK )
//                           flagEOT += 1 ;       /* flag last block read */
//                       }
//               }
//             else         /* Error, garbled ACK */
//               {
//               errors += 1 ;
//               if (first == NAK)
//                   printf("Error! Bad Sector # %d\n", snum ) ;
//                 else
//                   printf("Error! Garbled ACK, Sector # %d\n", snum ) ;
//               }
//           } while ( (nbytes > 0)&&(errors < errormax ) ) ;
//       }

//   Send( EOT ) ;
//   if ( (first = getDataTime(10)) != ACK )   /* One freebie NAK */
//      {
//      Send( EOT) ;
//      do {
//          if ( (first = getDataTime(10)) != ACK )
//              {
//              errors += 1 ;
//              printf("\nError! EOT not ACKed.  Recieved %xh\n", first );
//              Send( EOT ) ;                    /* resend on error */
//              }
//          } while ( (first != ACK)&&( errors < errormax )) ;
//      }

//   if (errors < errormax )
//       printf("\nTransfer complete") ;
//     else
//       printf("\nAborting");
//   ESCAPE:
//   fclose( fp ) ;
//   }


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
