#include "timezones.h"
#include <string.h>

struct Zone { const char *name; const char *posix; };
static const Zone kZones[] = {
    {"Los Angeles", "PST8PDT,M3.2.0,M11.1.0"},
    {"Denver", "MST7MDT,M3.2.0,M11.1.0"},
    {"Chicago", "CST6CDT,M3.2.0,M11.1.0"},
    {"New York", "EST5EDT,M3.2.0,M11.1.0"},
    {"UTC", "UTC0"},
    {"London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Dubai", "<+04>-4"},
    {"Singapore", "<+08>-8"},
    {"Tokyo", "JST-9"},
    {"Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
};
static const int kCount = sizeof(kZones) / sizeof(kZones[0]);

int timezoneCount(void) { return kCount; }
const char *timezoneName(int i) { return (i >= 0 && i < kCount) ? kZones[i].name : ""; }
const char *timezonePosix(int i) { return (i >= 0 && i < kCount) ? kZones[i].posix : "UTC0"; }
int timezoneDefault(void) { return 0; }

const char *timezoneOptions(void) {
  static char buf[256];
  static bool built = false;
  if (!built) {
    buf[0] = '\0';
    for (int i = 0; i < kCount; i++) {
      strncat(buf, kZones[i].name, sizeof(buf) - strlen(buf) - 1);
      if (i + 1 < kCount) strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);
    }
    built = true;
  }
  return buf;
}
