// SD card storage: profiles, selected profile, controller log, boot splash.
// Was gaggia_config.cpp. No heap allocation; fixed buffers; safe without a card.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "gaggia_state.h"
#include "BmpClass.h"

#include "config.h"  // SD pins

void initConfFile(GaggiaStateT *state, AdvancedSettingsT *advancedSettings);
bool storageReady();

int setupAndReadConfigFile();  // loads the selected profile into the state; 1 ok, -1 error
int writeConfigFile();         // saves the state into the selected profile

int logController(const char *message);  // appends "<time>,<message>" to the log
int deleteLogsFile();                     // starts a new log with the CSV header
void storageLogFlush();                   // closes the writer's handle (a reader may open the file)
const char *storageLogPath();

int displayFrankBmp(BMP_DRAW_CALLBACK *bmpDrawCallback, int16_t width, int16_t height);

// Profile names: at most PROFILE_NAME_MAX - 1 characters, no '/'. Longer names are
// truncated on write.
int listProfiles(char *buf, size_t size);       // ';'-separated names into buf; returns the count or -1
int getCurrentProfile(char *buf, size_t size);  // selected profile name into buf; 0 ok, -1 error
int writeCurrentProfile(const char *profileName);
int renameProfile(const char *newName);
bool deleteProfile(const char *profileToDelete);
int duplicateProfile();
