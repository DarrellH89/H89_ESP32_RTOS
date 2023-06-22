/******************************************************/
/* Define Statements for MT Modem
/* Created: 6/23/89
/* Last Modified: 7/4/89 - added XON, XOFF, BLKSIZE
/******************************************************/

#undef HDOS
#define H89
#undef SMALLZ



#define FALSE 0
#define TRUE 1
#define DEBUG FALSE

#define NUL  0             /* XMODEM constants */
#define SOH  1
#define STX  2
#define EOT  4
#define ACK  6
#define NAK  0x15
#define CAN  0x18
#define XON  0x11
#define XOFF 0x13
#define SUB  0x1A

#define TAB  9             /* Generic constants */
#define LF  0x0a
#define CR  0x0D
#define BS  0x08
#define Space ' '
#define DEL 0x7F
#define ESC 0x1b

#define RXMASK     1
#define TXMASK    20h

#ifdef H89
#define version "H-89 1.0 ESP 17 Jun 2023"
#define CTLPRT 0DH        /* Assembly language defintions */
#define TICCNT 0x0B
#define BLKSIZE 1024
#endif

#ifdef HDOS
#define version "H-89 HDOS ESP 1.0 17 Jun 2023"
#define CTLPRT 02036H           /* Assembly language defintions */
#define TICCNT 0x201B
#define BLKSIZE 512         /* disk block size */
#define DIRLEN    0x17
#endif

#define H89FLAG 1
#define fDATA 0E0H         /* H89 Serial port constants for M80*/
#define fCMD 0E2H          /* Command port */
#define fSTAT 0E1H         /* Status Port */
#define BASEPORT 0xE0         /* for C code */
#define CONPORT 0E8H         /* H-89 Console Port */
#define CAPKEY 'P'         /* Blue Func Key */
#define H88CTL 0F2H
#define SPDBIT 10H
#define NSPDBIT 0EFH

#define TICKSEC 500         /* 15 ticks/sec SmallZ80, 500 H89 */




#define lastbyte 128
#define timeout   -1
#define errormax  10
#define retrymax  10
#define BLOCK    4096       /* must be multiple of 1024 */
#define TBLOCK    4096        /* must be multiple of 1024 */
#define INITSZ 128

struct list {
    struct list *next ;
    char val[20];
    };
