#include "console.h"
#include "config.h"
#include "control.h"
#include "sensors.h"
#include "outputs.h"
#include "link.h"
#include "dfu_jump.h"

bool debugLog = false;  // off by default so the console replies are readable

void consoleSetup() {
  Serial.begin(CONSOLE_BAUD);
  delay(1000);
  Serial.println("serial works");
}

void readUsbCommand() {
  static char line[LINE_MAX];
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
      Serial.println(FIRMWARE_VERSION);
    } else if (strcmp(line, "STATUS") == 0) {
      printStatus();
    } else if (strncmp(line, "RX ", 3) == 0) {  // bench: feed a line as if from the screen
      char copy[LINE_MAX];
      strncpy(copy, line + 3, sizeof(copy) - 1);
      copy[sizeof(copy) - 1] = '\0';
      handleLine(copy);
      Serial.println("rx injected");
    } else if (strcmp(line, "LOG ON") == 0) {
      debugLog = true;
      Serial.println("log on");
    } else if (strcmp(line, "LOG OFF") == 0) {
      debugLog = false;
      Serial.println("log off");
    } else if (strcmp(line, "DFU") == 0) {
      allOutputsOff();
      Serial.println("rebooting into DFU bootloader");
      Serial.flush();
      delay(100);
      dfu_request_reboot();
    }
    // anything else is ignored (the debug port also receives stray text)
  }
}

void printStatus() {
  char line[224];
  snprintf(line, sizeof(line),
           "STATUS mode=%d tempSet=%.2f pressSet=%.2f pumpPct=%.2f temp=%.2f press=%.2f valve=%d boilerOut=%.1f pumpOut=%.1f tempFaults=%lu rx=%lu rxRejected=%lu rxOverflows=%lu loops=%lu maxLoopMs=%lu",
           (int)operating_mode, temperatureSetPoint, pressureSetPoint, pressureOutputPercent,
           temperature_smoothed, pressure_smoothed, valveIsOpen() ? 1 : 0, boiler_relay_output,
           pump_dimmer_output2, (unsigned long)temperatureFaults, (unsigned long)rxLines, (unsigned long)rxRejected,
           (unsigned long)rxOverflows, (unsigned long)loopCounter, (unsigned long)maxLoopMs);
  Serial.println(line);
}

void printSetPoints() {
  Serial.print("temperatureSetPoint ");
  Serial.print(temperatureSetPoint);
  Serial.print(" pressureSetPoint ");
  Serial.print(pressureSetPoint);
  Serial.print(" pressureOutputPercent ");
  Serial.println(pressureOutputPercent);
}

void printAdvancedSettings() {
  Serial.print(" advanced settings: ");
  Serial.print("boiler_bb_range:");
  Serial.print(boiler_bb_range);
  Serial.print(" boiler_PID_cycle:");
  Serial.print(boiler_PID_cycle);
  Serial.print(" boiler_PID_KP:");
  Serial.print(boiler_PID_KP);
  Serial.print(" boiler_PID_KI:");
  Serial.print(boiler_PID_KI);
  Serial.print(" boiler_PID_KD:");
  Serial.print(boiler_PID_KD);
  Serial.print(" pump_max_step_up:");
  Serial.print(pump_max_step_up);
  Serial.print(" pump_KP:");
  Serial.print(pump_KP);
  Serial.print(" pump_KI:");
  Serial.print(pump_KI);
  Serial.print(" pump_KD:");
  Serial.print(pump_KD);
  Serial.print(" unused1:");
  Serial.println(unused1);
}
