#include "gaggia_config.h"
#include <CSV_Parser.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "BmpClass.h"

static fs::FS* fileSystem;
static GaggiaStateT* state;
static AdvancedSettingsT* advancedSettings;

void initConfFile(GaggiaStateT* s, AdvancedSettingsT* as) {
  state = s;
  advancedSettings = as;
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

// ----------------------------------------------

int logController(const char* message) {
  const char* fileName = "/gaggia/gaggia_logs.csv";
  File logFile = fileSystem->open(fileName, FILE_APPEND);
  if (!logFile) {
    Serial.println("Failed to open file for appending");
    return -1;
  } else {
    return logFile.println(message);
  }
  //no closing... yep. really low tech
}

int deleteLogsFile() {
  const char* fileName = "/gaggia/gaggia_logs.csv";

  if (!fileSystem->remove(fileName)) {
    Serial.println("Failed to delete logs");
    return -1;
  }
  Serial.println("adding header to logs");
  return logController("sender;temperature;pressure;valve;boilerOutput;pumpOutput;temperatureSet;boilerBBRange;boilerPIDPeriod;boilerPIDKP;boilerPIDKI;boilerPIDKD;counter;|");
}

// ----------------------------------------------

int setupAndReadConfigFile() {

  const char* path = "/gaggia";
  Serial.println("about to open config file....");
  File gaggiaDir = fileSystem->open(path);
  Serial.println("opened config file....");
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
    writeConfigFile();
    return -1;
  } else {
    Serial.print("Read from file: ");
    if (file.available()) {
      String data = file.readString();
      Serial.println(data);
      char buffer[500];
      data.toCharArray(buffer, data.length() + 2);
      CSV_Parser cp(buffer, /*format*/ "sf");
      float* values = (float*)cp["value"];
      Serial.printf("%f , %f , %f", (float)values[0], (float)values[1], (float)values[2]);
      state->boilerSetPoint = (float)values[0];
      state->pressureSetPoint = (float)values[1];
      state->steamSetPoint = (float)values[2];
      state->steam_max_pressure = (float)values[3];
      state->steam_pump_output_percent = (float)values[4];
      state->blooming_pressure = (float)values[5];
      state->blooming_fill_time = (float)values[6];
      state->blooming_wait_time = (float)values[7];
      state->brew_timer = (float)values[8];
      state->hasConfigChanged = true;
      advancedSettings->boiler_bb_range = (float)values[9];
      advancedSettings->boiler_PID_cycle = (float)values[10];
      advancedSettings->boiler_PID_KP = (float)values[11];
      advancedSettings->boiler_PID_KI = (float)values[12];
      advancedSettings->boiler_PID_KD = (float)values[13];
      advancedSettings->pump_max_step_up = (float)values[14];
      advancedSettings->pump_KP = (float)values[15];
      advancedSettings->pump_KI = (float)values[16];
      advancedSettings->pump_KD = (float)values[17];
      advancedSettings->unused1 = (float)values[18];
      advancedSettings->userChanged = true;
      advancedSettings->sendToController = true;
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
  char buffer[500];
  const char* csv_str = "key,value\n"
                        "boilerSetPoint,%f\n"
                        "pressureSetPoint,%f\n"
                        "steamSetPoint,%f\n\n"
                        "steam_max_pressure,%f\n\n"
                        "steam_pump_output_percent,%f\n\n"
                        "blooming_pressure,%f\n\n"
                        "blooming_fill_time,%f\n\n"
                        "blooming_wait_time,%f\n\n"
                        "brew_timer,%f\n\n"
                        "boiler_bb_range,%f\n"
                        "boiler_PID_cicle,%f\n"
                        "boiler_PID_KP,%f\n"
                        "boiler_PID_KI,%f\n"
                        "boiler_PID_KD,%f\n"
                        "pump_max_step_up,%f\n"
                        "pump_KP,%f\n"
                        "pump_KI,%f\n"
                        "pump_KD,%f\n"
                        "unused1,%f\n";
  sprintf(buffer, csv_str,
          state->boilerSetPoint,
          state->pressureSetPoint,
          state->steamSetPoint,
          state->steam_max_pressure,
          state->steam_pump_output_percent,
          state->blooming_pressure,
          state->blooming_fill_time,
          state->blooming_wait_time,
          state->brew_timer,
          advancedSettings->boiler_bb_range,
          advancedSettings->boiler_PID_cycle,
          advancedSettings->boiler_PID_KP,
          advancedSettings->boiler_PID_KI,
          advancedSettings->boiler_PID_KD,
          advancedSettings->pump_max_step_up,
          advancedSettings->pump_KP,
          advancedSettings->pump_KI,
          advancedSettings->pump_KD,
          advancedSettings->unused1);
  Serial.println("writing to file");
  Serial.println(buffer);
  return writeConfigFile(buffer);
}

// -----------------------------------

static BmpClass bmpClass;

int displayFrankBmp(BMP_DRAW_CALLBACK* bmpDrawCallback, int16_t width, int16_t height) {
  const char* fileName = "/gaggia/frank.bmp";

  Serial.println("about to open frank file");
  File file = fileSystem->open(fileName, FILE_READ);
  if (!file) {
    Serial.println("Failed to open frank for displaying");
    return -1;
  } else {
    Serial.println("displaying frank");
    bmpClass.draw(
      &file, bmpDrawCallback, false /* useBigEndian */,
      0 /* x */, 0 /* y */, width /* widthLimit */, height /* heightLimit */);

    file.close();

  Serial.println("about to open config file....");
  File gaggiaDir = fileSystem->open("/gaggia");
  Serial.println("opened config file....");

    return 1;
  }
}