#include "net.h"
#include "timezones.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>
#include <esp_sntp.h>

#define NET_RECONNECT_MS 15000

static Preferences prefs;
static char ssid[33] = "";
static char password[65] = "";
static int tzIndex = 0;
static NetState state = NET_NO_CREDENTIALS;
static uint32_t lastAttempt = 0;
static bool timeValid = false;
static bool scanning = false;

static void applyTimezone() {
  configTzTime(timezonePosix(tzIndex), "pool.ntp.org", "time.nist.gov");
}

static void connect() {
  if (ssid[0] == '\0') {
    state = NET_NO_CREDENTIALS;
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("gaggiano");
  WiFi.begin(ssid, password);
  state = NET_CONNECTING;
  lastAttempt = millis();
  Serial.printf("wifi: connecting to %s\n", ssid);
}

void netBegin() {
  prefs.begin("gaggiano", false);
  prefs.getString("ssid", ssid, sizeof(ssid));
  prefs.getString("pass", password, sizeof(password));
  tzIndex = prefs.getInt("tz", timezoneDefault());
  if (tzIndex < 0 || tzIndex >= timezoneCount()) tzIndex = timezoneDefault();
  WiFi.persistent(false);  // we keep the credentials ourselves
  applyTimezone();
  connect();
}

void netPoll(uint32_t now) {
  wl_status_t ws = WiFi.status();
  if (state == NET_CONNECTING) {
    if (ws == WL_CONNECTED) {
      state = NET_CONNECTED;
      Serial.printf("wifi: connected, %s\n", WiFi.localIP().toString().c_str());
    } else if (now - lastAttempt > NET_RECONNECT_MS) {
      state = NET_FAILED;
      Serial.println("wifi: connection failed");
    }
  } else if (state == NET_CONNECTED) {
    if (ws != WL_CONNECTED) {
      Serial.println("wifi: lost, reconnecting");
      connect();
    }
  } else if (state == NET_FAILED) {
    if (now - lastAttempt > NET_RECONNECT_MS) connect();
  }
  if (!timeValid && state == NET_CONNECTED) {
    time_t t = time(NULL);
    if (t > 1700000000) {  // after 2023: the clock has been set
      timeValid = true;
      char buf[32];
      netLocalTime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S");
      Serial.printf("time: %s\n", buf);
    }
  }
}

void netGetStatus(struct NetStatus *out) {
  out->state = state;
  strncpy(out->ssid, ssid, sizeof(out->ssid) - 1);
  out->ssid[sizeof(out->ssid) - 1] = '\0';
  if (state == NET_CONNECTED) {
    strncpy(out->ip, WiFi.localIP().toString().c_str(), sizeof(out->ip) - 1);
    out->ip[sizeof(out->ip) - 1] = '\0';
    out->rssi = WiFi.RSSI();
  } else {
    out->ip[0] = '\0';
    out->rssi = 0;
  }
  out->timeValid = timeValid;
}

void netSetCredentials(const char *newSsid, const char *newPassword) {
  strncpy(ssid, newSsid, sizeof(ssid) - 1);
  ssid[sizeof(ssid) - 1] = '\0';
  strncpy(password, newPassword, sizeof(password) - 1);
  password[sizeof(password) - 1] = '\0';
  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  WiFi.disconnect(true);
  connect();
}

void netForget() {
  ssid[0] = '\0';
  password[0] = '\0';
  prefs.remove("ssid");
  prefs.remove("pass");
  WiFi.disconnect(true);
  state = NET_NO_CREDENTIALS;
}

int netScanStart() {
  if (scanning) return -1;
  WiFi.mode(WIFI_STA);
  WiFi.scanNetworks(true /* async */, false, false, 300);
  scanning = true;
  return 0;
}

int netScanCount() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return -1;
  if (n < 0) return 0;
  scanning = false;
  return n;
}

const char *netScanSsid(int i) {
  static char name[33];
  strncpy(name, WiFi.SSID(i).c_str(), sizeof(name) - 1);
  name[sizeof(name) - 1] = '\0';
  return name;
}

int netScanRssi(int i) { return WiFi.RSSI(i); }

void netSetTimezone(int index) {
  if (index < 0 || index >= timezoneCount()) return;
  tzIndex = index;
  prefs.putInt("tz", tzIndex);
  applyTimezone();
}

int netTimezone() { return tzIndex; }

bool netLocalTime(char *buf, size_t size, const char *fmt) {
  if (!timeValid) {
    strncpy(buf, "--:--", size - 1);
    buf[size - 1] = '\0';
    return false;
  }
  time_t t = time(NULL);
  struct tm tmv;
  localtime_r(&t, &tmv);
  strftime(buf, size, fmt, &tmv);
  return true;
}
