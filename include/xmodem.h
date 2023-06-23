/*****************************************************
 Define Statements for MT Modem
 Created: 6/23/89
 Last Modified: 7/4/89 - added XON, XOFF, BLKSIZE
******************************************************/

#undef HDOS

#define NUL 0 /* XMODEM constants */
#define SOH 1 // start header for 128 byte blocks
#define STX 2 // start header for 1024 blocks
#define SOH_Size 128
#define STX_Size 1024
#define SUB 0x1A
#define EOT 4
#define ACK 6
#define NAK 0x15
#define CAN 0x18
#define XON 0x11
#define XOFF 0x13

#define TAB 9 /* Generic constants */
#define LF 0x0a
#define CR 0x0D
#define BS 0x08
#define Space ' '
#define DEL 0x7F
#define ESC 0x1b
#define FALSE 0
#define TRUE 1

#define lastbyte 128
#define timeout -1
#define errormax 20
#define retrymax 10
#define BLOCK 4096  /* must be multiple of 1024 */
#define TBLOCK 4096 /* must be multiple of 1024 */
#define BLKSIZE 128