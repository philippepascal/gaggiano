#include "gaggia_state.h"
#include "BmpClass.h"

//remember SD library doesn't necesserily support all format. FAT works.
#define REASSIGN_PINS
#define SD_sck 12
#define SD_miso 13
#define SD_mosi 11
#define SD_cs 10


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

void initConfFile(GaggiaStateT *state, AdvancedSettingsT *advancedSettings);
int setupAndReadConfigFile();
int writeConfigFile();

// File setupLogFile();
int logController(const char *message);
int deleteLogsFile();

int displayFrankBmp(BMP_DRAW_CALLBACK *bmpDrawCallback, int16_t width, int16_t height);

char* listProfiles();
char* getCurrentProfile();
int writeCurrentProfile(const char* profileName);