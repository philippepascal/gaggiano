#include "link.h"
#include "config.h"
#include "control.h"
#include "sensors.h"
#include "outputs.h"
#include "console.h"
#include <gaggia_protocol.h>

uint32_t rxLines = 0, rxRejected = 0, rxOverflows = 0;
bool linkOk = false;

static HardwareSerial screenSerial(SCREEN_RX_PIN, SCREEN_TX_PIN);
static GpLineReader reader;
static uint32_t last_sent_message_time = 0;
static uint32_t last_cmd_time = 0;

static void sendMessage(const GpMessage &m) {
  char buf[GP_LINE_MAX];
  int n = gp_encode(&m, buf, sizeof(buf));
  if (n <= 0) return;
  screenSerial.write((const uint8_t *)buf, (size_t)n);
  if (debugLog) { Serial.print(" sent: "); Serial.print(buf); }
}

static void sendHello() {
  GpMessage m;
  m.type = GP_HELLO;
  m.hello.version = GP_PROTOCOL_VERSION;
  strncpy(m.hello.firmware, FIRMWARE_VERSION, GP_FIRMWARE_MAX - 1);
  m.hello.firmware[GP_FIRMWARE_MAX - 1] = '\0';
  sendMessage(m);
}

void linkSetup() {
  screenSerial.begin(SCREEN_BAUD);
  sendHello();
}

void pollScreenSerial() {
  while (screenSerial.available()) {
    if (reader.push((char)screenSerial.read())) {
      handleLine(reader.line(), reader.length());
    }
  }
  rxOverflows = reader.overflows();
}

static void applyCmd(const GpCmd &c) {
  if (c.mode < GP_MODE_OFF || c.mode > GP_MODE_CLEAN) return;
  operating_mode = c.mode;
  temperatureSetPoint = c.tempSet;
  pressureSetPoint = c.pressSet;
  pressureOutputPercent = c.pumpPct;
  last_cmd_time = millis();
  linkOk = true;
}

static void applyTune(const GpTune &t) {
  boiler_bb_range = t.bbRange;
  boiler_PID_cycle = t.pidCycle;
  boiler_PID_KP = t.kp;
  boiler_PID_KI = t.ki;
  boiler_PID_KD = t.kd;
  pump_max_step_up = t.pumpStepUp;
  pump_KP = t.pumpKp;
  pump_KI = t.pumpKi;
  pump_KD = t.pumpKd;
  steam_shot_s = t.steamShotS;
  steam_gap_s = t.steamGapS;
  updateAdvancedSettings();
  if (debugLog) printAdvancedSettings();
}

void handleLine(const char *line, size_t len) {
  GpMessage m;
  GpResult r = gp_decode(line, len, &m);
  if (r != GP_OK) {
    rxRejected++;
    if (debugLog) { Serial.print("rejected ("); Serial.print(gp_result_name(r)); Serial.print("): "); Serial.println(line); }
    return;
  }
  rxLines++;
  switch (m.type) {
    case GP_CMD:
      applyCmd(m.cmd);
      if (debugLog) printSetPoints();
      break;
    case GP_TUNE:
      applyTune(m.tune);
      break;
    case GP_HELLO:
      if (debugLog) { Serial.print("screen hello: "); Serial.println(m.hello.firmware); }
      sendHello();
      break;
    default:
      break;  // STAT from the screen makes no sense; ignore
  }
}

void checkLinkTimeout(uint32_t now) {
  // `now` is the loop start; a CMD applied later in the same pass stamps
  // last_cmd_time after it. Signed difference, so that case reads as "just now"
  // instead of wrapping to 49 days (2026-09-04: a 100 ms I2C timeout in the
  // pressure read made every heartbeat time out on arrival, mode forced off).
  if (linkOk && (int32_t)(now - last_cmd_time) > (int32_t)LINK_TIMEOUT_MS) {
    linkOk = false;
    operating_mode = OPERATING_MODE_OFF;  // pump off and valve closed on the next update
    pressureSetPoint = 0;
    pressureOutputPercent = 0;
    // temperatureSetPoint is kept on purpose: the boiler stays hot (decision D1)
    if (debugLog) Serial.println("link timeout: mode off");
  }
}

bool sendStatus(uint32_t now, uint32_t loopCounter, uint32_t maxLoopMsSinceLast) {
  if ((now - last_sent_message_time) > STATUS_SEND_PERIOD_MS) {
    GpMessage m;
    m.type = GP_STAT;
    m.stat.mode = (int)operating_mode;
    m.stat.temp = temperature_smoothed;
    m.stat.pressure = pressure_smoothed;
    m.stat.valve = valveIsOpen() ? 1 : 0;
    m.stat.boilerOut = boiler_relay_output;
    m.stat.pumpOut = pump_dimmer_output2;
    m.stat.tempSet = temperatureSetPoint;
    m.stat.pressSet = pressureSetPoint;
    m.stat.pumpPct = pressureOutputPercent;
    m.stat.linkOk = linkOk ? 1 : 0;
    m.stat.faults = temperatureFaults + pressureFaults;
    m.stat.counter = loopCounter;
    m.stat.pressStale = pressureStale ? 1 : 0;
    m.stat.i2cRecoveries = i2cRecoveries;
    m.stat.maxLoopMs = maxLoopMsSinceLast;
    sendMessage(m);
    last_sent_message_time = now;
    return true;
  }
  return false;
}
