#include "link.h"
#include "storage.h"
#include "config.h"
#include "console.h"
#include <gaggia_protocol.h>

extern bool isControllerLoggingOn;  // owned by the sketch: log raw STAT lines to SD

uint32_t linkRxLines = 0, linkRxRejected = 0, linkRxOverflows = 0, linkTxLines = 0;

static HardwareSerial *port = NULL;
static GaggiaStateT *state = NULL;
static AdvancedSettingsT *adv = NULL;
static GpLineReader reader;

static GpCmd current = {GP_MODE_OFF, 0, 0, 0};
static bool haveCommand = false;
static uint32_t lastCmdSent = 0;
static uint32_t lastStatAt = 0;
static int lastStatMode = -1;
static int mismatches = 0;  // consecutive STAT lines that disagree with `current`

static void sendMessage(const GpMessage &m) {
  char buf[GP_LINE_MAX];
  int n = gp_encode(&m, buf, sizeof(buf));
  if (n <= 0 || port == NULL) return;
  port->write((const uint8_t *)buf, (size_t)n);
  linkTxLines++;
  if (debugLog) {
    Serial.print(" sent: ");
    Serial.print(buf);
  }
}

static void sendHello() {
  GpMessage m;
  m.type = GP_HELLO;
  m.hello.version = GP_PROTOCOL_VERSION;
  strncpy(m.hello.firmware, SCREEN_FIRMWARE_VERSION, GP_FIRMWARE_MAX - 1);
  m.hello.firmware[GP_FIRMWARE_MAX - 1] = '\0';
  sendMessage(m);
}

static void sendCmd(uint32_t now) {
  GpMessage m;
  m.type = GP_CMD;
  m.cmd = current;
  sendMessage(m);
  lastCmdSent = now;
  mismatches = 0;
}

void linkSetup(HardwareSerial *serial, GaggiaStateT *s, AdvancedSettingsT *a) {
  port = serial;
  state = s;
  adv = a;
  sendHello();
}

void linkSetCommand(int mode, float tempSet, float pressSet, float pumpPct) {
  bool changed = !haveCommand || mode != current.mode || tempSet != current.tempSet ||
                 pressSet != current.pressSet || pumpPct != current.pumpPct;
  current.mode = mode;
  current.tempSet = tempSet;
  current.pressSet = pressSet;
  current.pumpPct = pumpPct;
  haveCommand = true;
  if (changed) sendCmd(millis());
}

void linkSendTune() {
  if (adv == NULL) return;
  GpMessage m;
  m.type = GP_TUNE;
  m.tune.bbRange = adv->boiler_bb_range;
  m.tune.pidCycle = adv->boiler_PID_cycle;
  m.tune.kp = adv->boiler_PID_KP;
  m.tune.ki = adv->boiler_PID_KI;
  m.tune.kd = adv->boiler_PID_KD;
  m.tune.pumpStepUp = adv->pump_max_step_up;
  m.tune.pumpKp = adv->pump_KP;
  m.tune.pumpKi = adv->pump_KI;
  m.tune.pumpKd = adv->pump_KD;
  sendMessage(m);
}

static bool nearlyEqual(float a, float b) { return fabsf(a - b) < 0.006f; }  // two decimals on the wire

static void applyStat(const GpStat &st, uint32_t now) {
  lastStatAt = now;
  lastStatMode = st.mode;
  state->tempRead = st.temp;
  state->pressureRead = st.pressure;
  state->isSolenoidOn = st.valve != 0;
  state->boilerOut = st.boilerOut;
  state->pumpOut = st.pumpOut;
  state->ctrlMode = st.mode;
  state->linkOk = st.linkOk != 0;
  if (!haveCommand) return;
  bool agrees = st.mode == current.mode && nearlyEqual(st.tempSet, current.tempSet) &&
                nearlyEqual(st.pressSet, current.pressSet) && nearlyEqual(st.pumpPct, current.pumpPct);
  if (agrees) {
    mismatches = 0;
  } else if (++mismatches >= 2) {
    Serial.println("controller state differs from the last command, re-sending");
    sendCmd(now);
  }
}

// Log rows: every status line while the controller runs something, one per second
// otherwise, so a day of idling stays small and a shot keeps its detail.
static void logStat(const GpStat &st, uint32_t now) {
  static uint32_t lastRow = 0;
  if (!isControllerLoggingOn) return;
  if (st.mode == GP_MODE_OFF && now - lastRow < 1000) return;
  lastRow = now;
  char row[160];
  snprintf(row, sizeof(row), "%d,%.2f,%.2f,%d,%.1f,%.1f,%.2f,%.2f,%.2f,%d,%lu,%lu", st.mode, st.temp, st.pressure,
           st.valve, st.boilerOut, st.pumpOut * 100.0f / 127.0f, st.tempSet, st.pressSet, st.pumpPct, st.linkOk,
           (unsigned long)st.faults, (unsigned long)st.counter);
  logController(row);
}

static void handleLine(const char *line, size_t len, uint32_t now) {
  GpMessage m;
  GpResult r = gp_decode(line, len, &m);
  if (r != GP_OK) {
    linkRxRejected++;
    Serial.print("rejected (");
    Serial.print(gp_result_name(r));
    Serial.print("): ");
    Serial.println(line);
    return;
  }
  linkRxLines++;
  switch (m.type) {
    case GP_STAT:
      applyStat(m.stat, now);
      logStat(m.stat, now);
      break;
    case GP_HELLO:
      Serial.print("controller hello: ");
      Serial.println(m.hello.firmware);
      linkSendTune();  // it rebooted with default tuning
      if (haveCommand) sendCmd(now);
      break;
    default:
      break;
  }
}

void linkPoll(uint32_t now) {
  if (port == NULL) return;
  while (port->available()) {
    if (reader.push((char)port->read())) handleLine(reader.line(), reader.length(), now);
  }
  linkRxOverflows = reader.overflows();
  if (haveCommand && (now - lastCmdSent) >= LINK_HEARTBEAT_MS) sendCmd(now);
}

bool linkControllerAlive(uint32_t now) {
  return lastStatAt != 0 && (now - lastStatAt) < LINK_STAT_TIMEOUT_MS;
}

int linkControllerMode() { return lastStatMode; }
