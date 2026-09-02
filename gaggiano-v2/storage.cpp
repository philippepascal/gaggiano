#include "storage.h"
#include "profile_format.h"
#include "net.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

static fs::FS *fileSystem = NULL;
static GaggiaStateT *state = NULL;
static AdvancedSettingsT *advancedSettings = NULL;

static const char *profilesPath = "/gaggia/profiles";
static const char *selectedProfilePath = "/gaggia/selectedProfile";
static const char *logsPath = "/gaggia/gaggia_logs.csv";
static const char *defaultProfile = "default.csv";

#define PATH_MAX_LEN 64

bool storageReady() { return fileSystem != NULL; }

void initConfFile(GaggiaStateT *s, AdvancedSettingsT *as) {
  state = s;
  advancedSettings = as;
  SPI.begin(SD_sck, SD_miso, SD_mosi, SD_cs);
  if (!SD.begin(SD_cs)) {
    Serial.println("SD: card mount failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("SD: no card attached");
    return;
  }
  Serial.printf("SD: %s, %llu MB\n",
                cardType == CARD_MMC ? "MMC" : cardType == CARD_SD ? "SDSC" : cardType == CARD_SDHC ? "SDHC" : "unknown",
                SD.cardSize() / (1024 * 1024));
  fileSystem = &SD;
}

// "/gaggia/profiles/<name>" with the name truncated to the profile limit and '/' removed.
static void profilePath(char *out, size_t size, const char *name) {
  char clean[PROFILE_NAME_MAX];
  size_t n = 0;
  for (const char *c = name; *c && n < PROFILE_NAME_MAX - 1; c++) {
    if (*c != '/' && *c != '\r' && *c != '\n') clean[n++] = *c;
  }
  clean[n] = '\0';
  snprintf(out, size, "%s/%s", profilesPath, clean);
}

static int writeFile(const char *fileName, const char *content) {
  File file = fileSystem->open(fileName, FILE_WRITE);
  if (!file) {
    Serial.printf("SD: cannot open %s for writing\n", fileName);
    return -1;
  }
  bool ok = file.print(content) > 0;
  file.close();
  if (!ok) Serial.printf("SD: write to %s failed\n", fileName);
  return ok ? 1 : -1;
}

// Reads up to size-1 bytes, NUL-terminates. Returns the length or -1.
static int readFile(const char *fileName, char *buf, size_t size) {
  File file = fileSystem->open(fileName, FILE_READ);
  if (!file) return -1;
  size_t n = file.readBytes(buf, size - 1);
  file.close();
  buf[n] = '\0';
  return (int)n;
}

// ---------------------------------------------------------------- logs

// The log file stays open between lines and is flushed once a second, instead
// of an open/append/close per line (five times a second while logging).
static File logFile;
static uint32_t lastLogFlush = 0;

int logController(const char *message) {
  if (!storageReady()) return -1;
  if (!logFile) {
    logFile = fileSystem->open(logsPath, FILE_APPEND);
    if (!logFile) {
      Serial.println("SD: cannot open the log for appending");
      return -1;
    }
  }
  char ts[24];
  if (!netLocalTime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S")) strcpy(ts, "-");
  logFile.print(ts);
  logFile.print(',');
  int n = logFile.println(message);
  uint32_t now = millis();
  if (now - lastLogFlush >= LOG_FLUSH_MS) {
    logFile.flush();
    lastLogFlush = now;
  }
  return n;
}

int deleteLogsFile() {
  if (!storageReady()) return -1;
  if (logFile) logFile.close();
  if (!fileSystem->remove(logsPath)) Serial.println("SD: no log file to delete");
  return logController("mode,temp,pressure,valve,heater,pump,tempSet,pressSet,pumpPct,linkOk,faults,counter");  // "time," is prepended
}

void storageLogFlush() {
  if (logFile) logFile.close();  // reopened by the next logController()
}

const char *storageLogPath() { return logsPath; }

// ---------------------------------------------------------------- profiles

int getCurrentProfile(char *buf, size_t size) {
  if (!storageReady()) return -1;
  int n = readFile(selectedProfilePath, buf, size);
  if (n < 0) {
    Serial.println("SD: no selected profile, creating it");
    writeCurrentProfile(defaultProfile);
    strncpy(buf, defaultProfile, size - 1);
    buf[size - 1] = '\0';
    return 0;
  }
  while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) buf[--n] = '\0';
  if (n == 0) {
    strncpy(buf, defaultProfile, size - 1);
    buf[size - 1] = '\0';
  }
  return 0;
}

int writeCurrentProfile(const char *profileName) {
  if (!storageReady()) return -1;
  return writeFile(selectedProfilePath, profileName);
}

int setupAndReadConfigFile() {
  if (!storageReady()) return -1;
  File dir = fileSystem->open(profilesPath);
  if (!dir) {
    if (!fileSystem->mkdir(profilesPath)) {
      Serial.println("SD: cannot create the profiles directory");
      return -1;
    }
  } else {
    dir.close();
  }
  char name[PROFILE_NAME_MAX];
  if (getCurrentProfile(name, sizeof(name)) < 0) return -1;
  char path[PATH_MAX_LEN];
  profilePath(path, sizeof(path), name);

  static char text[PROFILE_TEXT_MAX];
  if (readFile(path, text, sizeof(text)) < 0) {
    Serial.printf("SD: profile %s missing, creating it with the current values\n", name);
    strncpy(state->profile_name, name, PROFILE_NAME_MAX - 1);
    state->profile_name[PROFILE_NAME_MAX - 1] = '\0';
    writeConfigFile();
    return -1;
  }
  int applied = profileParse(text, state, advancedSettings);
  if (applied < 0) {
    Serial.printf("SD: profile %s is not a profile file, keeping current values\n", name);
    return -1;
  }
  strncpy(state->profile_name, name, PROFILE_NAME_MAX - 1);
  state->profile_name[PROFILE_NAME_MAX - 1] = '\0';
  state->hasConfigChanged = true;
  advancedSettings->userChanged = true;
  advancedSettings->sendToController = true;
  Serial.printf("SD: profile %s loaded, %d values, notes \"%s\"\n", name, applied, state->notes);
  return 1;
}

static int writeGivenConfigFile(const char *path) {
  static char text[PROFILE_TEXT_MAX];
  if (profileFormat(text, sizeof(text), state, advancedSettings) < 0) {
    Serial.println("SD: profile too long to write");
    return -1;
  }
  return writeFile(path, text);
}

int writeConfigFile() {
  if (!storageReady()) return -1;
  char name[PROFILE_NAME_MAX];
  if (getCurrentProfile(name, sizeof(name)) < 0) return -1;
  char path[PATH_MAX_LEN];
  profilePath(path, sizeof(path), name);
  return writeGivenConfigFile(path);
}

int listProfiles(char *buf, size_t size) {
  if (!storageReady() || size == 0) return -1;
  buf[0] = '\0';
  File dir = fileSystem->open(profilesPath);
  if (!dir) {
    Serial.println("SD: cannot open the profiles directory");
    return -1;
  }
  size_t used = 0;
  int count = 0;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    const char *name = entry.name();
    size_t len = strlen(name);
    if (used + len + 2 <= size) {  // name, ';', terminator
      memcpy(buf + used, name, len);
      used += len;
      buf[used++] = ';';
      buf[used] = '\0';
      count++;
    }
    entry.close();
  }
  dir.close();
  return count;
}

int renameProfile(const char *newName) {
  if (!storageReady()) return -1;
  char name[PROFILE_NAME_MAX];
  if (getCurrentProfile(name, sizeof(name)) < 0) return -1;
  char from[PATH_MAX_LEN], to[PATH_MAX_LEN];
  profilePath(from, sizeof(from), name);
  profilePath(to, sizeof(to), newName);
  if (!fileSystem->rename(from, to)) return -1;
  return writeCurrentProfile(to + strlen(profilesPath) + 1);  // the truncated, cleaned name
}

bool deleteProfile(const char *profileToDelete) {
  if (!storageReady()) return false;
  char path[PATH_MAX_LEN];
  profilePath(path, sizeof(path), profileToDelete);
  return fileSystem->remove(path);
}

int duplicateProfile() {
  if (!storageReady()) return -1;
  char name[PROFILE_NAME_MAX];
  if (getCurrentProfile(name, sizeof(name)) < 0) return -1;
  char copy[PROFILE_NAME_MAX + 4];
  snprintf(copy, sizeof(copy), "%s-c", name);
  char path[PATH_MAX_LEN];
  profilePath(path, sizeof(path), copy);
  return writeGivenConfigFile(path);
}

// ---------------------------------------------------------------- splash

static BmpClass bmpClass;

int displayFrankBmp(BMP_DRAW_CALLBACK *bmpDrawCallback, int16_t width, int16_t height) {
  if (!storageReady()) return -1;
  File optFile = fileSystem->open("/gaggia/frank_opt.bin", FILE_READ);
  File file = fileSystem->open("/gaggia/frank.bmp", FILE_READ);
  if (!file) {
    Serial.println("SD: no splash image");
    if (optFile) optFile.close();
    return -1;
  }
  bmpClass.draw(&file, &optFile, bmpDrawCallback, false /* useBigEndian */, 0, 0, width, height);
  file.close();
  if (optFile) optFile.close();
  return 1;
}
