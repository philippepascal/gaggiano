
//remember SD library doesn't necesserily support all format. FAT works.
#define REASSIGN_PINS
#define SD_sck 12
#define SD_miso 13
#define SD_mosi 11
#define SD_cs 10


/*********************
 *      DEFINES
 *********************/
//config and setting based
static bool hasChanged;
static double boilerSetPoint;
static double pressureSetPoint;
static double steamSetPoint;
//real time
static double tempRead;
static double pressureRead;
static bool isBoilerOn;
static bool isBrewing;
static bool isSteaming;

/**********************
 *      TYPEDEFS
 **********************/

void initConfFile();
int setupAndReadConfigFile();
int writeConfigFile();