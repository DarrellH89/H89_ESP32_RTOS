#include "settings.h"

//****************** SD Test ************
void sdTest(){
  //  SPIClass spi = SPIClass(VSPI);
  //  spi.begin(SD_CLK,SD_OUT, SD_IN, SD_CS);
   if(!SD.begin()){     //SD_CS,spi,80000000)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    listDir(SD, "/", 0);
    createDir(SD, "/mydir");
    listDir(SD, "/", 0);
    removeDir(SD, "/mydir");
    listDir(SD, "/", 2);
    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    deleteFile(SD, "/foo.txt");
    renameFile(SD, "/hello.txt", "/foo.txt");
    readFile(SD, "/foo.txt");
    testFileIO(SD, "/test.txt");
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    File file = SD.open("/win89_turbo.img");
    byte diskType = 0;
    if(file){
      if(file.seek(5, SeekSet)){
        file.read(&diskType,1);
        Serial.printf("Byte 5 %x\n", diskType );
      }
        else
          Serial.printf("Seek Failed\n");
    }
    else
      Serial.printf("Win89 Read Error\n");
}