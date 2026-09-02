// Time zones offered on the WiFi view: a name and the POSIX TZ string the ESP32's
// clock understands. Pure table, host-tested.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
int timezoneCount(void);
const char *timezoneName(int index);   // "Los Angeles", ...
const char *timezonePosix(int index);  // "PST8PDT,M3.2.0,M11.1.0", ...
int timezoneDefault(void);             // index of the default zone
// A newline-separated list of the names for an LVGL dropdown (static buffer).
const char *timezoneOptions(void);
#ifdef __cplusplus
}
#endif
