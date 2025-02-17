extern "C" {
#include "gaggia_config.h"
}
#include <CSV_Parser.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

static fs::FS* fileSystem;
static GaggiaStateT* state;

void initConfFile(GaggiaStateT* s) {
  state = s;
  SPI.begin(SD_sck, SD_miso, SD_mosi, SD_cs);
  Serial.println("SPI initialized");
  if (!SD.begin(SD_cs)) {
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
    state->boilerSetPoint = 99;
    state->pressureSetPoint = 7.0;
    state->steamSetPoint = 143;
    writeConfigFile();
    return -1;
  } else {
    Serial.print("Read from file: ");
    if (file.available()) {
      String data = file.readString();
      Serial.println(data);
      char buffer[200];
      data.toCharArray(buffer, data.length()+2);
      CSV_Parser cp(buffer, /*format*/ "sf");
      float* values = (float*)cp["value"];
      Serial.printf("%f , %f , %f",(float)values[0],(float)values[1],(float)values[2]);
      state->boilerSetPoint = (float)values[0];
      state->pressureSetPoint = (float)values[1];
      state->steamSetPoint = (float)values[2];
      state->hasChanged = true;
      Serial.println("state updated");
    } else {
      Serial.print("CSV parsing failed");
    }
    file.close();
    return 1;
  }
}

int writeConfigFile(const char* content) {
  const char* fileName = "/gaggia/gaggia_settings.csv";
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
  char buffer[200];
  const char* csv_str = "key,value\n"
                        "boilerSetPoint,%f\n"
                        "pressureSetPoint,%f\n"
                        "steamSetPoint,%f\n\n";
  sprintf(buffer, csv_str, state->boilerSetPoint, state->pressureSetPoint, state->steamSetPoint);
  Serial.println("writing to file");
  Serial.println(buffer);
  return writeConfigFile(buffer);
}