#include "console.h"
#include "config.h"
#include "gaggia_state.h"
#include "link.h"
#include "sequencer.h"
#include "storage.h"
#include <gaggia_protocol.h>

extern GaggiaStateT state;
extern bool isControllerLoggingOn;

bool debugLog = true;
static uint32_t lastHeapReport = 0;

void consoleSetup() {
  Serial.begin(CONSOLE_BAUD);
  Serial.println("Gaggiano screen " SCREEN_FIRMWARE_VERSION);
}

static void printStatus(uint32_t now) {
  Serial.printf("STATUS profile=%s heat=%d brew=%d steam=%d clean=%d bloom=%d auto=%d phase=%d "
                "controller=%s ctrlMode=%d temp=%.2f press=%.2f valve=%d rx=%lu rxRejected=%lu rxOverflows=%lu tx=%lu "
                "sd=%d sdlog=%d heap=%u minheap=%u\n",
                state.profile_name, state.isBoilerOn, state.isBrewing, state.isSteaming, state.isCleaning,
                state.isBlooming, state.isAuto, sequencerPhase(), linkControllerAlive(now) ? "alive" : "silent",
                linkControllerMode(), state.tempRead, state.pressureRead, state.isSolenoidOn,
                (unsigned long)linkRxLines, (unsigned long)linkRxRejected, (unsigned long)linkRxOverflows,
                (unsigned long)linkTxLines, storageReady(), isControllerLoggingOn, (unsigned)ESP.getFreeHeap(),
                (unsigned)ESP.getMinFreeHeap());
}

void consolePoll(uint32_t now) {
  static char line[64];
  static uint8_t len = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (len < sizeof(line) - 1) line[len++] = c;
      continue;
    }
    line[len] = '\0';
    len = 0;
    if (strcmp(line, "VERSION") == 0) {
      Serial.println(SCREEN_FIRMWARE_VERSION);
    } else if (strcmp(line, "STATUS") == 0) {
      printStatus(now);
    } else if (strcmp(line, "LOG ON") == 0) {
      debugLog = true;
      Serial.println("log on");
    } else if (strcmp(line, "LOG OFF") == 0) {
      debugLog = false;
      Serial.println("log off");
    } else if (strcmp(line, "SDLOG ON") == 0) {
      isControllerLoggingOn = true;
      Serial.println("sd log on");
    } else if (strcmp(line, "SDLOG OFF") == 0) {
      isControllerLoggingOn = false;
      Serial.println("sd log off");
    }
  }
  if (now - lastHeapReport >= HEAP_REPORT_MS) {
    lastHeapReport = now;
    Serial.printf("HEAP free=%u minfree=%u psramfree=%u\n", (unsigned)ESP.getFreeHeap(),
                  (unsigned)ESP.getMinFreeHeap(), (unsigned)ESP.getFreePsram());
  }
}
