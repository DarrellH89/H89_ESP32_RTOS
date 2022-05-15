# H89_ESP32
Interface code to support Heathkit H-89 using the ESP32 as a file server


The Heathkit H-89 is a 1980 era Z-80 based computer running CP/M or HDOS. The interface hardware provides a bidirectional data port at 0xFC, a ESP32 status port at 0xFD, and a wake-up signal for the ESP32 at port 0x7E.

The ESP32 hosts two webpages. One for over the Air updates and another to manage files stored on a micro SD card. The host network ID and password are stored in non-volatile memory on the ESP32. Files for use by the H-89 can be uploaded and download through the webpage by the ESP32.

A prototype board is currently undergoing testing.

Current project status is in the Status directory.

The command interface between the H-89 and ESP32 is listed below:

Web Features:
* Webpage listing files on micro SD card with ability to upload/download/delete
* Ability to reboot ESP32
* Over The Air updates for firmware

Commands:
Implemented
* List Files on the micro SD card
* Debug - simple get four bytes from H89, add 1 to each byte and send them back
* Get file from H89 using Xmodem protocol with CRC
* Reboot ESP32 by writing to Command port twice

Under Development
* Read Status of last operation
* Read sectors using Cylinder/Head/Sector (CHS)
* Write Sectors using CHS
* Select disk image using text file name
* Select disk image using file number
* Read sectors using LBA
* Write Sectors using LBA
