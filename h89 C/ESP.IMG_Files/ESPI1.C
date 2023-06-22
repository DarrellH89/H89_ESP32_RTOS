/* MT Modem include file routines */
/* uses port in Setup() and DTR and CTS functions */

#include "printf.h"
#include "esp.h"

/*
#ifdef HDOS
#include "hdos.h"
#endif
*/

/* These globals are used in other modules   */
/* Modify .MAC file to add names so the EXTRN command can find them */

extern unsigned *ticks;
extern char statline[] ;
extern int baudrate ;        /* global baud rate divisor */
extern int baud;               /* global baud speed */
extern char port;         /* Serial port */
extern char fastFlag;
int tranSize = 1024;        /* Default packet size */
char xyModem = 'X';        /* Default Xmodem */

#ifdef SMALLZ
char Curstr[] = { ESC, '[', ' ',';', ' ','f', 0 } ;        /* cursor string */
char ClrEOL[] = { ESC, '[','0','K', 0 } ;         /* Clear to End of Line
char ClrScr[] = { ESC, '[', '2', 'J', 0 }  ;           /* Clear the screen *
char SaveCur[] = { ESC, '7', 0 }  ;      /* Save the current cursor position */
char RetCur[] = { ESC, '8', 0 }    ;      /* Restore to previous saved pos. */
char On25[] =    { ESC, 'x', '1', 0 } ;         /* Enable 25th line  */
char Off25[] =    { ESC, 'y', '1', 0 }  ;      /* Disable 25th line  */
#else
char Curstr[] = { ESC, 'Y', ' ', ' ', 0 } ;        /* cursor string */
char ClrEOL[] = { ESC, '[','K', 0 } ;             /* Clear to End of Line */
char ClrScr[] = { ESC, '[','E', 0 }  ;             /* Clear the screen */
char SaveCur[] = { ESC, 'j', 0 }  ;      /* Save the current cursor position */
char RetCur[] = { ESC, 'k', 0 }    ;      /* Restore to previous saved pos. */
char On25[] =    { ESC, 'x', '1', 0 } ;         /* Enable 25th line  */
char Off25[] =    { ESC, 'y', '1', 0 }  ;      /* Disable 25th line  */
#endif



/*************** File List *********************/
/* Get File list from ESP32*/
FileList()
    {
    int i ;
    char *ptr, *ptrb ;


    return 0 ;
    }



/********************* SetYmodem *********************/
setYmodem()
    {
    char ch ;

    do
    {
    printf("\n");
    printf("Options? 1) Transfer Block 128 bytes\n");
    printf("     2) Transfer Block 1024 bytes\n");
    printf("     3) Xmodem\n");
    printf("     4) Ymodem\n");
    printf("     0) Exit\n",);
    printf("    ?: ");

    do ch = GetCon() ;
       while (ch == 0) ;
    putchar('\n');
    switch (ch)
       {
       case '1':
            tranSize = 128;
            printf("Block size: %d\n",tranSize);
            break ;
       case '2':
            tranSize = 1024;
            printf("Block size: %d\n",tranSize);
            break ;
       case '3':
            xyModem = 'X';
            printf("Set for Xmodem\n");
            break ;
       case '4':
            xyModem = 'Y';
            printf("Set for Ymodem\n");
            break ;
       }
    } while(ch != '0')   ;

    }
/********************** FlushCon ******************
/* flush console
/* intput: none
/* output: void
*/
FlushCon()
    {
    int ch ;

    while((ch=GetCon())!= 0);
    }



/*********************    Output Status ***************************/

Ostatus()
  {
#asm
    .Z80
osport:     in      a, (fSTAT)
     and   3h
     ld    l,a
     ld    h,0
    .8080
#endasm
  }


/*********************    Send ***************************/

Send( ch )
    char  ch;
    {
/*    int status, timeout;

    timeout = 1000;
    do
        status = Ostatus();
    while (status == 1 || status == 3);   /* wait until ready */
#asm
    .Z80
sdstat:
     in   a, (fSTAT)
     and  1
     jr   nz, sdstat
     ld   hl,2
     add  hl,sp
     ld   a,(hl)
sdport:
     out   (fDATA),a
     .8080
#endasm
    }

/************** StartCmd ***************/\
/* Write to Command Port */

StartCmd()
{
#asm
.z80
    ld   a,1
    out (fCMD), a
    .8080
#endasm

}

/*********************    rcvchar ***************************/

rcvchar()
  {
#asm
    .z80
isport:
    in     a, (fSTAT)
    and    3
    ld     hl, -1
    cp     1   ; okay to read data
    ret    nz
rcport:
    in      a, (fDATA)
    ld      h,0
    ld      l,a
    .8080
#endasm
    }



/*********************    Readline ***************************/

Readline( cnt )
  unsigned cnt;
  {
  int m ;

  cnt = cnt * TICKSEC  ;  /* H89 = 500 number of 2ms in a
                 second plus *ticks
                 /* Small Z80 tick = 65.2 ms, 16 /sec */
  *ticks = 0;
  m  = -1;             /* error value */
  while( cnt > *ticks )
      if( (m = rcvchar()) != -1)
      break;
  return m  ;
  }


/*********************    GetCon ***************************/

GetCon()
    {
/*  HDOS */
#ifdef HDOS
#asm
    SCALL    MACRO    TYPE
        DB    377Q,TYPE
    ENDM

    scall    1    ; check console stat, return 0 if not ready
    lxi    h,0
    rc
    mov    l,a    ; Had a char, so return it in HL
#endasm
#else
/* CP/M    */
#asm
    .z80
    ld    e,0ffh    ; check for input
    ld    c,6    ; BDOS function 6
    call    5
    ld    hl,0    ; make sure hl = 0
    ld    l,a    ; Make L = A, 0 if no char
    .8080
#endasm
#endif
    }

/*********************    SendText ***************************/
SendText( str )
  char *str;
  {
  while ( *str != 0 )
      Send( str++ );
  }




/*************** putcon *****************************/
/* Put char c out port e8h  */

putcon( c )
  char c ;
  {
  putc(c);
/*
#asm
    .z80
    ld    hl,4        ; get the pointer to the string
    call    mtoffset
putc1:
    in    a,(CONPORT+5)    ; check console status
    and    20h
    jr    z,putc1     ; loop until console ready
    ld    a,(hl)
    out    (CONPORT),a
putce:    ld    hl,0        ; return success
;     ret
    .8080
#endasm
*/
  }

/*************** puts *****************************/
/* Put a string S to port e8h until 0 */

puts( s )
  char *s ;
  {
/*  printf("%s", s);
/*/
#asm
    .z80
    ld    hl,4        ; get the pointer to the string
    call    mtoffset
puts1:    in    a,(CONPORT+5)    ; check console status
    and    20h
    jr    z,puts1     ; loop until console ready
    ld    a,(hl)
    or    a        ; check for 0
    jr    z,putse     ; done
    out    (CONPORT),a
    inc    hl        ; point to next
    jr    puts1
putse:    ld    hl,0        ; return success
;     ret
;
mtoffset::
    add    hl,sp        ; HL contains offset to value in stack
    ld    a,(hl)        ; get LS byte
    inc    hl        ; point to MS byte
    ld    h,(hl)        ; get MS byte
    ld    l,a        ; load LS byte into L
    ret
    .8080
#endasm
  }



/*************** goto XY *****************************/
gotoXY( x,y )
   int x, y;        /* x = col, y = row     */
    {
/*    printf("%c[%d;%df",ESC,x,y);
*/
#asm
        .z80
        ld      hl,6            ; get x
        call    mtoffset
        ld      de,31           ; add video offset
        add     hl,de
        ld      a,l             ; save the x value
        ld      (curstr+3),a
        ld      hl,4            ; get y
        call    mtoffset
        ld      de,31
        add     hl,de           ; add video offset for y
        ld      a,l
        ld      (curstr+2),a
        ld      hl,curstr       ; get address of XY cursor string
        push    hl              ; put on stack for puts()
        call    puts
        pop     bc              ; fix stack

    ld    hl,0        ; return 0
    ret
    .8080
#endasm
}
  ; load LS byte into L
    ret
    .8080
#endasm
  }



/*************** goto XY *****************************/
gotoXY( x,y )
   int x, y;        /* x = col, y = row     */
    {
/*    printf("%c[%d;%df",ESC,x,y);
*/
#asm
        .z80
        ld      hl,6            ; get x
        call    mtoffset
        ld      de,31           ; add video offset
        add     hl,de
        ld      a,l             ; save the x value
        ld      (curstr+3),a
        ld      hl,4            ; ge