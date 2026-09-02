#include "ota.h"
#include "config.h"
#include "gaggia_state.h"
#include "link.h"
#include "net.h"
#include "sequencer.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <lvgl.h>
#include <gaggia_protocol.h>

extern GaggiaStateT state;
extern "C" void ui_show_notice(const char *text);
extern "C" void ui_hide_notice(void);

static bool started = false;
static char password[33] = OTA_DEFAULT_PASSWORD;

void otaBegin() {
  Preferences prefs;
  prefs.begin("gaggiano", true);
  prefs.getString("otapass", password, sizeof(password));
  prefs.end();
  ArduinoOTA.setHostname("gaggiano");
  ArduinoOTA.setPassword(password);
  ArduinoOTA.setMdnsEnabled(false);  // webui.cpp owns mDNS
  ArduinoOTA.onStart([]() {
    // stop whatever is running before the flash is rewritten
    state.isBrewing = state.isSteaming = state.isCleaning = state.isBlooming = state.isAuto = false;
    state.isBoilerOn = false;
    sequencerReset();
    linkSetCommand(GP_MODE_OFF, 0, 0, 0);
    Serial.println("ota: start");
    ui_show_notice("Updating firmware...");
    lv_timer_handler();
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    static unsigned lastPct = 100;
    unsigned pct = total ? done * 100 / total : 0;
    if (pct / 5 != lastPct / 5) {
      lastPct = pct;
      char text[48];
      snprintf(text, sizeof(text), "Updating firmware %u%%", pct);
      ui_show_notice(text);
      lv_timer_handler();
    }
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("ota: done, rebooting");
    ui_show_notice("Update done, restarting");
    lv_timer_handler();
  });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("ota: error %d\n", (int)e);
    ui_show_notice("Update failed");
    lv_timer_handler();
    delay(1500);
    ui_hide_notice();
  });
}

void otaPoll(uint32_t now) {
  (void)now;
  struct NetStatus st;
  netGetStatus(&st);
  if (st.state == NET_CONNECTED && !started) {
    ArduinoOTA.begin();
    started = true;
    Serial.println("ota: ready on port 3232");
  }
  if (started) ArduinoOTA.handle();
}
