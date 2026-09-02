// USB serial console (from the Mac). One command per line: VERSION, STATUS,
// LOG ON|OFF (console chatter: sent lines, LVGL logs), SDLOG ON|OFF (raw STAT
// lines to the SD card). Non-blocking.
#pragma once
#include <Arduino.h>

extern bool debugLog;

void consoleSetup();
void consolePoll(uint32_t now);  // reads commands, prints the HEAP line every HEAP_REPORT_MS
