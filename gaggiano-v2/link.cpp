#include "link.h"
#include "storage.h"
#include "config.h"
#include "console.h"
#include <gaggia_protocol.h>
#include <stdarg.h>

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
static bool wasAlive = false;
static bool wasPressStale = false;

// Event row in the SD log, next to the STAT rows: "<time>,#,<text>". No commas in
// the text so the CSV keeps its shape; readers skip rows whose second field is '#'.
static void logEvent(const char *fmt, ...) {
  if (!isControllerLoggingOn) return;
  char row[160];
  int n = snprintf(row, sizeof(row), "#,");
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(row + n, sizeof(row) - n, fmt, ap);
  va_end(ap);
  logController(row);
}

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

// `reason` NULL for the plain heartbeat (not logged); anything else goes to the log.
static void sendCmd(uint32_t now, const char *reason) {
  GpMessage m;
  m.type = GP_CMD;
  m.cmd = current;
  sendMessage(m);
  lastCmdSent = now;
  mismatches = 0;
  if (reason != NULL)
    logEvent("cmd %s mode=%d temp=%.2f press=%.2f pump=%.2f", reason, current.mode, (double)current.tempSet,
             (double)current.pressSet, (double)current.pumpPct);
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
  if (changed) sendCmd(millis(), "change");
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
  logEvent("tune bb=%.2f cycle=%.0f kp=%.3f ki=%.3f kd=%.3f step=%.3f pkp=%.3f pki=%.3f pkd=%.3f",
           adv->boiler_bb_range, adv->boiler_PID_cycle, adv->boiler_PID_KP, adv->boiler_PID_KI, adv->boiler_PID_KD,
           adv->pump_max_step_up, adv->pump_KP, adv->pump_KI, adv->pump_KD);
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
  state->pressStale = st.pressStale != 0;
  if (state->pressStale != wasPressStale) {
    wasPressStale = state->pressStale;
    logEvent(state->pressStale ? "pressure sensor stale (faults=%lu recoveries=%lu)" : "pressure sensor live (faults=%lu recoveries=%lu)",
             (unsigned long)st.faults, (unsigned long)st.i2cRecoveries);
  }
  if (!haveCommand) return;
  bool agrees = st.mode == current.mode && nearlyEqual(st.tempSet, current.tempSet) &&
                nearlyEqual(st.pressSet, current.pressSet) && nearlyEqual(st.pumpPct, current.pumpPct);
  if (agrees) {
    mismatches = 0;
  } else if (++mismatches >= 2) {
    Serial.println("controller state differs from the last command, re-sending");
    logEvent("controller echoes mode=%d temp=%.2f press=%.2f pump=%.2f", st.mode, (double)st.tempSet,
             (double)st.pressSet, (double)st.pumpPct);
    sendCmd(now, "mismatch");
  }
  // Heartbeats are flowing but the controller says it sees none: it is dropping
  // or undoing them (2026-09-04: a slow loop tripped its timeout on every CMD).
  static uint32_t lastLinkWarning = 0;
  if (st.linkOk == 0 && now - lastCmdSent < 2 * LINK_HEARTBEAT_MS && now - lastLinkWarning >= 5000) {
    lastLinkWarning = now;
    Serial.println("controller reports the link down while heartbeats are being sent");
    logEvent("controller reports link down while heartbeats are sent");
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
  snprintf(row, sizeof(row), "%d,%.2f,%.2f,%d,%.1f,%.1f,%.2f,%.2f,%.2f,%d,%lu,%lu,%d,%lu,%lu", st.mode, st.temp,
           st.pressure, st.valve, st.boilerOut, st.pumpOut * 100.0f / 127.0f, st.tempSet, st.pressSet, st.pumpPct,
           st.linkOk, (unsigned long)st.faults, (unsigned long)st.counter, st.pressStale,
           (unsigned long)st.i2cRecoveries, (unsigned long)st.maxLoopMs);
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
    // one row per second at most: a bad wire would otherwise flood the card
    static uint32_t lastRejectLog = 0, rejectedSinceLog = 0;
    rejectedSinceLog++;
    if (now - lastRejectLog >= 1000) {
      logEvent("rejected %lu line(s) (last: %s)", (unsigned long)rejectedSinceLog, gp_result_name(r));
      lastRejectLog = now;
      rejectedSinceLog = 0;
    }
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
      logEvent("controller hello v%d %s", m.hello.version, m.hello.firmware);
      linkSendTune();  // it rebooted with default tuning
      if (haveCommand) sendCmd(now, "hello");
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
  if (haveCommand && (now - lastCmdSent) >= LINK_HEARTBEAT_MS) sendCmd(now, NULL);
  bool alive = linkControllerAlive(now);
  if (alive != wasAlive) {
    wasAlive = alive;
    logEvent(alive ? "controller answering" : "controller silent");
  }
  if (state != NULL) state->ctrlAlive = alive;
}

bool linkControllerAlive(uint32_t now) {
  return lastStatAt != 0 && (now - lastStatAt) < LINK_STAT_TIMEOUT_MS;
}

int linkControllerMode() { return lastStatMode; }
