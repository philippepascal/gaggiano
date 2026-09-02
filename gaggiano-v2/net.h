// WiFi, credentials and time for the screen. The C part is what the UI uses; the
// simulator provides a stub of it (sim/net_stub.cpp).
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum NetState { NET_NO_CREDENTIALS = 0, NET_CONNECTING = 1, NET_CONNECTED = 2, NET_FAILED = 3 };

struct NetStatus {
  int state;       // NetState
  char ssid[33];   // saved network, empty if none
  char ip[16];
  int rssi;        // dBm, 0 when not connected
  bool timeValid;  // the clock has been set by NTP
};

void netGetStatus(struct NetStatus *out);
void netSetCredentials(const char *ssid, const char *password);  // saves, then connects
void netForget(void);                                           // clears the credentials, disconnects
int netScanStart(void);                                         // 0 started, -1 busy
int netScanCount(void);                                         // -1 while scanning, else the count
const char *netScanSsid(int index);
int netScanRssi(int index);
void netSetTimezone(int index);  // index in timezones.h; saved
int netTimezone(void);
// Local time through strftime; false (and "--:--") when the clock is not valid yet.
bool netLocalTime(char *buf, size_t size, const char *fmt);

#ifdef __cplusplus
}
void netBegin();
void netPoll(uint32_t now);
#endif
