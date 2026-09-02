#include "webui.h"
#include "config.h"
#include <lvgl.h>
#include "gaggia_state.h"
#include "history.h"
#include "link.h"
#include "net.h"
#include "sequencer.h"
#include "storage.h"
#include "web_page.h"
#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SD.h>
#include <Update.h>
#include <Preferences.h>
#include <gaggia_protocol.h>

extern "C" void ui_show_notice(const char *text);
extern "C" void ui_hide_notice(void);
static char otaPassword[33] = OTA_DEFAULT_PASSWORD;

extern GaggiaStateT state;

static WebServer server(80);
static bool mdnsUp = false;

// JSON string escaping for the few free-text fields (profile name, notes).
static void jsonString(char *out, size_t size, const char *in) {
  size_t n = 0;
  for (const char *c = in; *c && n + 2 < size; c++) {
    if (*c == '"' || *c == '\\') { out[n++] = '\\'; out[n++] = *c; }
    else if ((unsigned char)*c < 0x20) { out[n++] = ' '; }
    else out[n++] = *c;
  }
  out[n] = '\0';
}

static const char *phaseName() {
  int ph = sequencerPhase();
  if (ph == PHASE_BLOOM_FILL) return "prime";
  if (ph == PHASE_BLOOM_WAIT) return "wait";
  if (ph == PHASE_BREW || state.isBrewing) return "brew";
  if (state.isCleaning) return "clean";
  if (state.isSteaming) return "steam";
  return "";
}

static void handleStatus() {
  char profile[64], notes[256], clock[24];
  jsonString(profile, sizeof(profile), state.profile_name);
  jsonString(notes, sizeof(notes), state.notes);
  char *dot = strrchr(profile, '.');
  if (dot && dot != profile) *dot = '\0';
  netLocalTime(clock, sizeof(clock), "%Y-%m-%d %H:%M:%S");
  float timer = 0;
  if (state.actionStartTime > 0) {
    uint32_t stop = state.actionStopTime > 0 ? (uint32_t)state.actionStopTime : millis();
    timer = (stop - (uint32_t)state.actionStartTime) / 1000.0f;
  }
  char body[900];
  snprintf(body, sizeof(body),
           "{\"profile\":\"%s\",\"notes\":\"%s\",\"time\":\"%s\","
           "\"temp\":%.2f,\"pressure\":%.2f,\"valve\":%d,\"heater\":%.1f,\"pump\":%.1f,"
           "\"tempSet\":%.2f,\"pressSet\":%.2f,\"ctrlMode\":%d,\"link\":%d,"
           "\"heat\":%d,\"brew\":%d,\"steam\":%d,\"clean\":%d,\"prime\":%d,\"auto\":%d,"
           "\"phase\":\"%s\",\"timer\":%.1f,\"lastShot\":%.1f,\"heap\":%u}",
           profile, notes, clock, state.tempRead, state.pressureRead, state.isSolenoidOn ? 1 : 0, state.boilerOut,
           state.pumpOut * 100.0f / 127.0f, state.boilerSetPoint, state.pressureSetPoint, state.ctrlMode,
           state.linkOk ? 1 : 0, state.isBoilerOn, state.isBrewing, state.isSteaming, state.isCleaning,
           state.isBlooming, state.isAuto, phaseName(), timer, state.lastBrewTime, (unsigned)ESP.getFreeHeap());
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", body);
}

// One JSON array from a history ring, streamed in chunks.
static void sendArray(const char *name, int id, float scale, bool isInt, bool last) {
  char chunk[512];
  int n = history_count(id), len = 0;
  len += snprintf(chunk + len, sizeof(chunk) - len, "\"%s\":[", name);
  for (int i = 0; i < n; i++) {
    float v = history_at(id, i) * scale;
    len += isInt ? snprintf(chunk + len, sizeof(chunk) - len, "%d%s", (int)v, i + 1 < n ? "," : "")
                 : snprintf(chunk + len, sizeof(chunk) - len, "%.1f%s", v, i + 1 < n ? "," : "");
    if (len > (int)sizeof(chunk) - 16) {
      server.sendContent(chunk, len);
      len = 0;
    }
  }
  len += snprintf(chunk + len, sizeof(chunk) - len, "]%s", last ? "" : ",");
  server.sendContent(chunk, len);
}

static void handleHistory() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", "");
  server.sendContent("{");
  sendArray("temp", HISTORY_G_TEMP, 1.0f, false, false);
  sendArray("pressure", HISTORY_G_PRESS, 1.0f, false, false);
  sendArray("heater", HISTORY_G_BOILER, 1.0f, false, false);
  sendArray("pump", HISTORY_G_PUMP, 1.0f, false, false);
  sendArray("mode", HISTORY_G_MODE, 1.0f, true, true);
  server.sendContent("}");
  server.sendContent("");
}

static void handleLog() {
  if (!storageReady()) { server.send(503, "text/plain", "no SD card"); return; }
  storageLogFlush();  // the writer's handle is closed so the reader sees everything
  File f = SD.open(storageLogPath(), FILE_READ);
  if (!f) { server.send(404, "text/plain", "no log yet"); return; }
  server.sendHeader("Content-Disposition", "attachment; filename=\"gaggiano-log.csv\"");
  server.streamFile(f, "text/csv");
  f.close();
}

// ---- firmware update by HTTP upload (POST /update, multipart "firmware", field "password").
// A push model: it works when the sender can reach the screen, which is the direction
// that works across an IoT network. The panel shows the progress.

static bool updateAuthorized = false;
static bool updateFailed = false;

static void updateUpload() {
  HTTPUpload &up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    updateAuthorized = server.hasArg("password") && server.arg("password") == otaPassword;
    updateFailed = false;
    if (!updateAuthorized) return;
    // stop whatever is running before the flash is rewritten
    state.isBrewing = state.isSteaming = state.isCleaning = state.isBlooming = state.isAuto = false;
    state.isBoilerOn = false;
    sequencerReset();
    linkSetCommand(GP_MODE_OFF, 0, 0, 0);
    Serial.printf("update: start (%s)\n", up.filename.c_str());
    ui_show_notice("Updating firmware...");
    lv_timer_handler();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { updateFailed = true; Update.printError(Serial); }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (!updateAuthorized || updateFailed) return;
    if (Update.write(up.buf, up.currentSize) != up.currentSize) { updateFailed = true; Update.printError(Serial); }
    static uint32_t lastShown = 0;
    if (millis() - lastShown > 500) {
      lastShown = millis();
      char text[48];
      snprintf(text, sizeof(text), "Updating firmware %u KB", (unsigned)(up.totalSize / 1024));
      ui_show_notice(text);
      lv_timer_handler();
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (!updateAuthorized || updateFailed) return;
    if (Update.end(true)) {
      Serial.printf("update: %u bytes, restarting\n", (unsigned)up.totalSize);
      ui_show_notice("Update done, restarting");
    } else {
      updateFailed = true;
      Update.printError(Serial);
    }
    lv_timer_handler();
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    updateFailed = true;
  }
}

static void updateDone() {
  if (!updateAuthorized) { server.send(403, "text/plain", "wrong password"); return; }
  if (updateFailed || Update.hasError()) {
    ui_show_notice("Update failed");
    lv_timer_handler();
    delay(1500);
    ui_hide_notice();
    server.send(500, "text/plain", "update failed");
    return;
  }
  server.send(200, "text/plain", "ok, restarting");
  delay(300);
  ESP.restart();
}

static void handleIndex() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", WEB_INDEX_HTML);
}

void webBegin() {
  server.on("/", handleIndex);
  server.on("/api/status", handleStatus);
  server.on("/api/history", handleHistory);
  server.on("/logs.csv", handleLog);
  server.on("/update", HTTP_POST, updateDone, updateUpload);
  Preferences prefs;
  prefs.begin("gaggiano", true);
  prefs.getString("otapass", otaPassword, sizeof(otaPassword));
  prefs.end();
  server.onNotFound([]() { server.send(404, "text/plain", "not found"); });
  server.begin();
}

void webPoll(uint32_t now) {
  static uint32_t lastCheck = 0;
  if (now - lastCheck >= 1000) {
    lastCheck = now;
    struct NetStatus st;
    netGetStatus(&st);
    if (st.state == NET_CONNECTED && !mdnsUp) {
      mdnsUp = MDNS.begin("gaggiano");
      if (mdnsUp) MDNS.addService("http", "tcp", 80);
      Serial.println(mdnsUp ? "web: http://gaggiano.local" : "web: mDNS failed");
    } else if (st.state != NET_CONNECTED && mdnsUp) {
      MDNS.end();
      mdnsUp = false;
    }
  }
  server.handleClient();
}
