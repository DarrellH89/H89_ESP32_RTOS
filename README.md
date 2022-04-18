# H89_ESP32
Interface code to support Heathkit H-89 using the ESP32 as a file server


The Heathkit H-89 is a 1980 era Z-80 based computer running CP/M or HDOS. The interface hardware provides a bidirectional data port at 0xFC, a ESP32 status port at 0xFD, and a wake-up signal for the ESP32 at port 0x7E.

The ESP32 hosts two webpages. One for over the Air updates and another to manage files stored on a micro SD card. The host network ID and password are stored in non-volatile memory on the ESP32. Files for use by the H-89 can be uploaded and download through the webpage by the ESP32.

A prototype board is currently undergoing testing.

Current project status is in the Status directory.

The command interface between the H-89 and ESP32 is listed below:

Commands:

Read Status of last operation

Read sectors using Cylinder/Head/Sector (CHS)

Write Sectors using CHS

List Files on the micro SD card

Select disk image using text file name

Select disk image using file number

Read sectors using LBA

Write Sectors using LBA
