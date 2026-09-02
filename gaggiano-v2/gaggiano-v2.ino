// Gaggiano screen: ESP32-S3 with a 4.3" touch panel (Sunton ESP32-8048S043).
// LVGL user interface, brew profiles and logs on the SD card, and the serial
// link to the STM32 controller. See docs/ in the repository root.
//
// Modules: config.h (pins, periods), display_glue (panel, LVGL drivers, touch,
// splash), lv_buildUI (the UI), storage (SD card), sequencer (bloom/auto/brew
// phases), link (controller protocol), console (USB commands).

#include <lvgl.h>
#include <gaggia_protocol.h>
#include "config.h"
#include "gaggia_state.h"
#include "display_glue.h"
#include "lv_buildUI.h"
#include "storage.h"
#include "link.h"
#include "sequencer.h"
#include "console.h"
#include "net.h"

GaggiaStateT state = { false, 98, 8.0, 134, 0, 0, 0, 0, 0, 0, false, "", "", 0, 0, false, 0, false, false, false, false, false, false, false, false, 0, 0 };
AdvancedSettingsT advancedSettings = { false, false, 3, 1000, 10, 0.2, 0.1, 1, 100, 1, 0.1, 0.05 };
bool isControllerLoggingOn = false;  // raw STAT lines to the SD card (console: SDLOG ON)

HardwareSerial controllerSerial(2);

void setup() {
  consoleSetup();
  controllerSerial.setRxBufferSize(CONTROLLER_RX_BUFFER);
  controllerSerial.begin(CONTROLLER_BAUD, SERIAL_8N1, CONTROLLER_RX_PIN, CONTROLLER_TX_PIN);
  linkSetup(&controllerSerial, &state, &advancedSettings);
  linkSetCommand(GP_MODE_OFF, 0, 0, 0);  // initial command: everything off; starts the 1 s heartbeat
  netBegin();

  if (!displaySetup()) return;  // no display: keep the link and console alive anyway

  initConfFile(&state, &advancedSettings);
  displaySplash();

  instantiateUI(&state, &advancedSettings, writeConfigFile, listProfiles, getCurrentProfile,
                writeCurrentProfile, setupAndReadConfigFile, renameProfile, deleteProfile, duplicateProfile);
  setupAndReadConfigFile();
  deleteLogsFile();
  Serial.println("Setup done");
}

static uint32_t lastUiRefresh = 0;

void loop() {
  uint32_t now = millis();
  linkPoll(now);
  netPoll(now);
  consolePoll(now);
  if (now - lastUiRefresh >= UI_REFRESH_MS) {
    lastUiRefresh = now;
    updateUI();
  }
  lv_timer_handler();  // let the GUI do its work

  SeqCommand c;
  if (sequencerStep(&state, now, &c)) linkSetCommand(c.mode, c.tempSet, c.pressSet, c.pumpPct);
  if (advancedSettings.sendToController) {
    linkSendTune();
    advancedSettings.sendToController = false;
  }
  delay(5);
}

// Images: converted with the LVGL image converter v8, https://lvgl.io/tools/imageconverter
