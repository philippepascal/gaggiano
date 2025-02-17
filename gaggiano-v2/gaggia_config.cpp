extern "C" {
#include "gaggia_config.h"
}
#include <CSV_Parser.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

static fs::FS* fileSystem;

void initConfFile() {

#ifdef REASSIGN_PINS
  SPI.begin(SD_sck, SD_miso, SD_mosi, SD_cs);
  if (!SD.begin(SD_cs)) {
#else
  if (!SD.begin()) {
#endif
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  fileSystem = &SD;
}

int setupAndReadConfigFile() {

  const char* path = "/gaggia";
  File gaggiaDir = fileSystem->open(path);
  if (!gaggiaDir) {
    Serial.print("can't open /gaggia, trying to create it");
    if (!fileSystem->mkdir(path)) {
      Serial.print("Dir creation failed...");
    } else {
      gaggiaDir = fileSystem->open(path);
      if (!gaggiaDir) {
        Serial.print("can't open /gaggia after creating it...");
        return -1;
      }
    }
  }

  const char* fileName = "/gaggia/gaggia_settings.csv";
  File file = fileSystem->open(fileName);
  if (!file) {
    Serial.println("Failed to open file for reading, trying to create it with default values");
    boilerSetPoint = 99;
    pressureSetPoint = 7.0;
    steamSetPoint = 143;
    writeConfigFile();
    return -1;
  } else {
    Serial.print("Read from file: ");
    char r = file.read();
    char* s;
    while (file.available())
      s += r;
    CSV_Parser cp(s, /*format*/ "sd");
    double* values = (double*)cp["value"];
    boilerSetPoint = (double)values[0];
    pressureSetPoint = (double)values[1];
    steamSetPoint = (double)values[2];
    hasChanged = true;

    file.close();
    return 1;
  }
}

int writeConfigFile(const char* content) {
  const char* fileName = "/gaggia/gaggia_settins.csv";
  File file = fileSystem->open(fileName, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return -1;
  }
  if (file.print(content)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
  return 1;
}

int writeConfigFile() {
  char buffer[100];
  const char* csv_str = "key,value\n"
                        "boilerSetPoint,%d/n"
                        "pressureSetPoint,%d/n"
                        "steamSetPoint,%d/n";
  sprintf(buffer, csv_str, boilerSetPoint, pressureSetPoint, steamSetPoint);
  return writeConfigFile(csv_str);
}