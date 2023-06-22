/**************************************************
/* ESP32 Interface for HDOS and CP/M
/* Author: Darrell Pelan
/* written for Software Toolworks C80
/* Created 3/6/87
/* Last Modified:  17 Jun 23 modiofied for ESP32 communication
/*    12/20/2022 Modified for Small Z80, Added Ymodem
/*    6/26/89 - add Send File Xmodem mode
/*    6/19/89 - add Conference mode
/*    4/10/89 - ver 0.6, recv XMODEM, Terminal
/*
/**************************************************/

#include "esp.h"
#include "printf.h"
/* comment to fix strange compiler behavior */

unsigned *ticks;
int baudrate ;         /* global baud rate divisor */
char port;           /* current modem port */
char fastFlag;          /* FLag to slow down for serial port reads */
int baud;      /* current modem speed */
char *rcvbuf, *rcvptr;
char statline[80];   /* status line data */

extern int tranSize;
extern char xyModem;
extern char Curstr[] ;           /* cursor string */
extern char RetCur[] ;
extern char SaveCur[] ;
extern char ClrEOL[] ;
extern char ClrScr[] ;
extern char On25[] ;
extern char Off25[] ;

/*********************  Main Modem    *********************************/



main()
  {
  int j, fp;
  char buf[256];
  static int ok;
  char option;


  init() ;

  do
    {
      switch( option = GetOption() )
      {
        case 'L': FileList();         break;
        case 'R': ReadFile()  ;      break;
        case 'S': SendFile()  ;      break ;
        case 'Y': SetYmodem() ;      break;
        case 'D': Debug(); break;
        case '1': printf("Status: %d\n", Ostatus()); break;
        case '2': StartCmd();StartCmd(); break;
      }
    }
    while (option != 'X') ;
  puts( Off25 ) ;

  return 0;
  }

/*********************    GetOption ***************************/

int GetOption()
    {
    char ch;

/*    printf("%cE", ESC );   /* clear the screen */
    FlushCon();
    printf("\n\nESP32, %s\n",version);
    printf("Options:\n\n");
    printf("  R - receive a file\n");
    printf("  S - send a file\n");
    printf("  Y - X/Ymodem Options (Block %d, ", tranSize);
    if( xyModem == 'X')
    printf("Xmodem)\n");
      else
    printf("Ymodem)\n");
    printf("  D - Debug Test\n");
    printf("  L - Get ESP32 File List\n");
    printf("  1 - Get ESP32 Status\n");
    printf("  2 - Reset ESP32\n");

    printf("  X - exit to system\n");
    printf("\nwhich ? ");
    do ch = toupper( GetCon() );     /* direct console I/O */
    while (ch == 0);
    putchar('\n');
    return ch;
    }

/*********************    Debug ***************************/
Debug()
  {
  char ans ;
  char buf[10];
  int j, try, cnt, bptr;

  printf("Command Port = %x\n", BASEPORT);
  do
  {
      FlushCon();
      bptr = 0;
      printf("Sending Command 1\n");
      StartCmd();
      Delay(5);
      Send(1);
      Send(5);
      Send(18);
      Send(44);
      try = 40;
      for (cnt = 0; cnt < 4; cnt++)
      {
          do
              {
              j = rcvchar();
              if( j < 0 )
                  try--;
                else
                  try = 0;
              } while ( try > 0 );

          if (j > 0)
              buf[bptr++] = j;
          else
              printf("Data Read Error\n");
      }
      for (cnt = 0; cnt < bptr; cnt++)
          printf("Data: %d\n", buf[cnt]);
      printf("Send Command Again ? ");
      ans = toupper(getchar());
      printf("\n");
      } while(ans == 'Y');
  return 0;
  }

/********************* File List ************************/
/* Get file list from ESP32 */

FileList()
{
    char get;
    int bptr, len, ptr, err, j;
    int result;

    StartCmd();
    Send(0x10);
    rcvptr = rcvbuf;    /* reset buffer */
    len = 0;
    err = 0;
    bptr = 0;
    get = 1;
    do
        if ((get = rcvchar()) > 0)
        {
            *(rcvptr + bptr++) = get;
            if (bptr == BLOCK)
            {
                for (j = 0; j < BLOCK; j++)
                    putchar(*(rcvptr + j ));
                len += (bptr - 1);
                bptr = 0;
            }
            if (get == 10)
                *(rcvptr + bptr++) = 0x0d;
        }
      else
        {
            if (err % 10 == 0)
                printf("File List Read Error %c\n", get);
            err++;
            if (err > 200)
                {
                    get = 0;
                    printf("Buf ptr: %d data: %d\n", bptr, *(rcvptr + bptr));
                }
        }
      while( get != 0 );
      len += ( bptr - 1);
      if (bptr > 0 )
          for( j = 0; j < bptr; j++)
              putchar(*(rcvptr + j));
      printf("\nLen: %d\n", len);
}

/********************** Statline **********************/
/* Write Status line on 25th line; uses global "statline" */

Stat25()
  {
  puts( SaveCur ) ;
  puts( On25 ) ;
  gotoXY( 1,25 );
      /* Reverse Video, statline, stop reverse video, Erase to EOL */
  printf("%c%cp%s%cq%cK", CR, ESC, statline, ESC, ESC  );
  puts( RetCur ) ;
#ifdef HDOS
  set_cline();
#endif
  }


/********************** Init ****************************/
/* initialize the system and globals */

init()
    {
    ticks = TICCNT;
    port = BASEPORT;
    fastFlag = 1;
    tranSize = 128;

    rcvbuf = alloc( BLOCK + 256 );  /* 128 for Ymodem filename, 256 HDOS */
    rcvptr = rcvbuf;


    if( rcvbuf == -1)
        {
        printf("Not enough memory - TERMINATED\n");
        exit();
        }
#ifdef HDOS
   set_cline() ;
#endif
#ifdef H89
    printf("%cv", ESC );    /* set terminal wrap mode */
#endif
    }


/****** Interrupt functions *****/
ei()
{
#asm
    ei
#endasm
}

di()
{
#asm
    di
#endasm
}
