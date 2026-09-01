#include "link.h"
#include "config.h"
#include "control.h"
#include "sensors.h"
#include "outputs.h"
#include "console.h"

uint32_t rxLines = 0, rxRejected = 0, rxOverflows = 0;

static HardwareSerial screenSerial(SCREEN_RX_PIN, SCREEN_TX_PIN);
static char rxLine[LINE_MAX];
static uint8_t rxLen = 0;
static bool rxDiscard = false;  // true while skipping the rest of an oversize line
static uint32_t last_sent_message_time = 0;

void linkSetup() {
  screenSerial.begin(SCREEN_BAUD);
  screenSerial.println("hello screen");
}

void pollScreenSerial() {
  while (screenSerial.available()) {
    char c = (char)screenSerial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (!rxDiscard && rxLen > 0) {
        rxLine[rxLen] = '\0';
        handleLine(rxLine);
      }
      rxLen = 0;
      rxDiscard = false;
      continue;
    }
    if (rxDiscard) continue;
    if (rxLen < LINE_MAX - 1) {
      rxLine[rxLen++] = c;
    } else {
      rxOverflows++;
      rxDiscard = true;
    }
  }
}

// Splits "t;f;f;...;|" in place. Returns the field count after the type
// (-1 if malformed), type in *type, values in out[].
static int parseFields(char *line, int *type, double *out, int maxOut) {
  char *end = strchr(line, '|');
  if (end == NULL) return -1;
  *end = '\0';
  char *tok = line;
  int n = -1;  // -1: the type field has not been read yet
  while (tok != NULL && *tok != '\0') {
    char *sep = strchr(tok, ';');
    if (sep != NULL) *sep = '\0';
    if (n < 0) {
      *type = atoi(tok);
    } else {
      if (n >= maxOut) return -1;
      out[n] = atof(tok);
    }
    n++;
    tok = (sep != NULL) ? sep + 1 : NULL;
  }
  return n;
}

void handleLine(char *line) {
  double f[MAX_FIELDS];
  int type = -1;
  int n = parseFields(line, &type, f, MAX_FIELDS);
  bool ok = true;
  if (type == 1 && n == 2) {  // brew: temp, pressure
    operating_mode = OPERATING_MODE_BREW;
    temperatureSetPoint = f[0];
    pressureSetPoint = f[1];
    pressureOutputPercent = 0;
  } else if (type == 2 && n == 3) {  // steam: temp, max pressure, pump percent
    operating_mode = OPERATING_MODE_STEAM;
    temperatureSetPoint = f[0];
    pressureSetPoint = f[1];
    pressureOutputPercent = f[2];
  } else if (type == 3 && n == 2) {  // clean: temp, pressure
    operating_mode = OPERATING_MODE_CLEAN;
    temperatureSetPoint = f[0];
    pressureSetPoint = f[1];
    pressureOutputPercent = 0;
  } else if (type == 9 && n == 10) {  // advanced settings
    boiler_bb_range = f[0];
    boiler_PID_cycle = f[1];
    boiler_PID_KP = f[2];
    boiler_PID_KI = f[3];
    boiler_PID_KD = f[4];
    pump_max_step_up = f[5];
    pump_KP = f[6];
    pump_KI = f[7];
    pump_KD = f[8];
    unused1 = f[9];
    updateAdvancedSettings();
    if (debugLog) printAdvancedSettings();
  } else {
    ok = false;
  }
  if (ok) {
    rxLines++;
    if (debugLog && type != 9) printSetPoints();
  } else {
    rxRejected++;
    if (debugLog) { Serial.print("rejected type "); Serial.print(type); Serial.print(" fields "); Serial.println(n); }
  }
}

bool sendStatus(uint32_t now, uint32_t loopCounter) {
  if ((now - last_sent_message_time) > STATUS_SEND_PERIOD_MS) {
    char message[100] = "";
    snprintf(message, sizeof(message), "0;%.2f;%.2f;%d;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%lu;|",
             temperature_smoothed,
             pressure_smoothed,
             valveIsOpen() ? 1 : 0,
             boiler_relay_output,
             pump_dimmer_output2,
             temperatureSetPoint,
             boiler_bb_range,
             boiler_PID_cycle,
             boiler_PID_KP,
             boiler_PID_KI,
             boiler_PID_KD,
             (unsigned long)loopCounter);
    if (debugLog) { Serial.print(" sent: "); Serial.println(message); }
    screenSerial.println(message);
    last_sent_message_time = now;
    return true;
  }
  return false;
}
