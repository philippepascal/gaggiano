// Simulator stand-in for net.cpp: a fake network that connects two seconds after
// credentials are set, three networks in the scan, the host clock for the time.
#include "net.h"
#include "timezones.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>

static char ssid[33] = "";
static int state = NET_NO_CREDENTIALS;
static int tz = 0;
static double connectAt = 0;
static int scanState = 0;  // 0 idle, 1 running

static double nowS() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count() / 1000.0;
}

void netGetStatus(struct NetStatus *out) {
  if (state == NET_CONNECTING && nowS() > connectAt) state = NET_CONNECTED;
  out->state = state;
  strncpy(out->ssid, ssid, sizeof(out->ssid) - 1);
  out->ssid[32] = '\0';
  strcpy(out->ip, state == NET_CONNECTED ? "192.168.1.42" : "");
  out->rssi = state == NET_CONNECTED ? -58 : 0;
  out->timeValid = state == NET_CONNECTED;
}
void netSetCredentials(const char *s, const char *p) {
  (void)p;
  strncpy(ssid, s, 32);
  ssid[32] = '\0';
  state = NET_CONNECTING;
  connectAt = nowS() + 2;
  std::printf("[net] credentials set for '%s'\n", ssid);
}
void netForget(void) { ssid[0] = '\0'; state = NET_NO_CREDENTIALS; }
int netScanStart(void) { scanState = 1; connectAt = connectAt; return 0; }
int netScanCount(void) {
  static double since = 0;
  if (scanState == 1) { since = nowS(); scanState = 2; return -1; }
  if (scanState == 2 && nowS() - since < 1.5) return -1;
  scanState = 0;
  return 3;
}
const char *netScanSsid(int i) { static const char *n[] = {"Kitchen", "Neighbour-5G", "CoffeeLab"}; return (i >= 0 && i < 3) ? n[i] : ""; }
int netScanRssi(int i) { static const int r[] = {-48, -71, -83}; return (i >= 0 && i < 3) ? r[i] : 0; }
void netSetTimezone(int index) { tz = index; }
int netTimezone(void) { return tz; }
bool netLocalTime(char *buf, size_t size, const char *fmt) {
  if (state != NET_CONNECTED) { strncpy(buf, "--:--", size - 1); buf[size - 1] = '\0'; return false; }
  std::time_t t = std::time(nullptr);
  std::strftime(buf, size, fmt, std::localtime(&t));
  return true;
}
// Simulator scenes preload a network.
extern "C" void net_stub_preset_connected(void) { strcpy(ssid, "Kitchen"); state = NET_CONNECTED; }
