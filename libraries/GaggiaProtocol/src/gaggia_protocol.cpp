#include "gaggia_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GP_MAX_FIELDS 12

static const char *const kTypeNames[] = {"?", "HELLO", "STAT", "CMD", "TUNE"};
static const int kFieldCounts[] = {0, 2, 12, 4, 9};

const char *gp_type_name(GpType t) {
  return (t >= GP_HELLO && t <= GP_TUNE) ? kTypeNames[t] : "?";
}

const char *gp_result_name(GpResult r) {
  switch (r) {
    case GP_OK: return "ok";
    case GP_ERR_FRAME: return "frame";
    case GP_ERR_CHECKSUM: return "checksum";
    case GP_ERR_TYPE: return "type";
    case GP_ERR_FIELDS: return "fields";
    case GP_ERR_LENGTH: return "length";
  }
  return "?";
}

uint8_t gp_checksum(const char *payload, size_t len) {
  uint8_t x = 0;
  for (size_t i = 0; i < len; i++) x ^= (uint8_t)payload[i];
  return x;
}

// ---------------------------------------------------------------- encode

int gp_encode(const GpMessage *m, char *buf, size_t bufSize) {
  if (bufSize < 8) {
    if (bufSize) buf[0] = '\0';
    return -1;
  }
  // Build the payload (between '$' and '*') first, then frame it.
  char payload[GP_LINE_MAX];
  int n;
  switch (m->type) {
    case GP_HELLO:
      n = snprintf(payload, sizeof(payload), "HELLO,%d,%s", m->hello.version, m->hello.firmware);
      break;
    case GP_STAT:
      n = snprintf(payload, sizeof(payload), "STAT,%d,%.2f,%.2f,%d,%.1f,%.2f,%.2f,%.2f,%.2f,%d,%lu,%lu",
                   m->stat.mode, (double)m->stat.temp, (double)m->stat.pressure, m->stat.valve ? 1 : 0,
                   (double)m->stat.boilerOut, (double)m->stat.pumpOut, (double)m->stat.tempSet,
                   (double)m->stat.pressSet, (double)m->stat.pumpPct, m->stat.linkOk ? 1 : 0,
                   (unsigned long)m->stat.faults, (unsigned long)m->stat.counter);
      break;
    case GP_CMD:
      n = snprintf(payload, sizeof(payload), "CMD,%d,%.2f,%.2f,%.2f", m->cmd.mode, (double)m->cmd.tempSet,
                   (double)m->cmd.pressSet, (double)m->cmd.pumpPct);
      break;
    case GP_TUNE:
      n = snprintf(payload, sizeof(payload), "TUNE,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
                   (double)m->tune.bbRange, (double)m->tune.pidCycle, (double)m->tune.kp, (double)m->tune.ki,
                   (double)m->tune.kd, (double)m->tune.pumpStepUp, (double)m->tune.pumpKp,
                   (double)m->tune.pumpKi, (double)m->tune.pumpKd);
      break;
    default:
      buf[0] = '\0';
      return -1;
  }
  if (n < 0 || (size_t)n >= sizeof(payload)) {
    buf[0] = '\0';
    return -1;
  }
  // "$" + payload + "*HH\n" + NUL
  if ((size_t)n + 6 > bufSize || (size_t)n + 5 > GP_LINE_MAX) {
    buf[0] = '\0';
    return -1;
  }
  int total = snprintf(buf, bufSize, "$%s*%02X\n", payload, gp_checksum(payload, (size_t)n));
  return total;
}

// ---------------------------------------------------------------- decode

static int hexval(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static GpType typeFromName(const char *s, size_t len) {
  for (int t = GP_HELLO; t <= GP_TUNE; t++) {
    if (strlen(kTypeNames[t]) == len && memcmp(kTypeNames[t], s, len) == 0) return (GpType)t;
  }
  return GP_UNKNOWN;
}

static float toFloat(const char *s) { return (float)strtod(s, NULL); }
static int toInt(const char *s) { return (int)strtol(s, NULL, 10); }
static uint32_t toU32(const char *s) { return (uint32_t)strtoul(s, NULL, 10); }

GpResult gp_decode(const char *line, size_t len, GpMessage *out) {
  out->type = GP_UNKNOWN;
  if (len > GP_LINE_MAX) return GP_ERR_LENGTH;
  // Trim trailing CR/LF, then find the frame.
  while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
  const char *start = (const char *)memchr(line, '$', len);
  if (start == NULL) return GP_ERR_FRAME;
  len -= (size_t)(start - line);
  const char *payload = start + 1;
  size_t plen = len - 1;
  if (plen < 4 || payload[plen - 3] != '*') return GP_ERR_FRAME;
  int h = hexval(payload[plen - 2]), l = hexval(payload[plen - 1]);
  if (h < 0 || l < 0) return GP_ERR_FRAME;
  plen -= 3;
  if ((uint8_t)((h << 4) | l) != gp_checksum(payload, plen)) return GP_ERR_CHECKSUM;

  // Copy the payload so it can be split in place.
  char work[GP_LINE_MAX];
  memcpy(work, payload, plen);
  work[plen] = '\0';

  // Split on commas: fields[0] is the type word.
  char *fields[GP_MAX_FIELDS + 1];
  int n = 0;
  char *p = work;
  while (p != NULL) {
    if (n > GP_MAX_FIELDS) return GP_ERR_FIELDS;
    fields[n++] = p;
    char *c = strchr(p, ',');
    if (c != NULL) *c = '\0';
    p = (c != NULL) ? c + 1 : NULL;
  }
  GpType type = typeFromName(fields[0], strlen(fields[0]));
  if (type == GP_UNKNOWN) return GP_ERR_TYPE;
  if (n - 1 != kFieldCounts[type]) return GP_ERR_FIELDS;
  char **f = fields + 1;
  switch (type) {
    case GP_HELLO:
      out->hello.version = toInt(f[0]);
      strncpy(out->hello.firmware, f[1], GP_FIRMWARE_MAX - 1);
      out->hello.firmware[GP_FIRMWARE_MAX - 1] = '\0';
      break;
    case GP_STAT:
      out->stat.mode = toInt(f[0]);
      out->stat.temp = toFloat(f[1]);
      out->stat.pressure = toFloat(f[2]);
      out->stat.valve = toInt(f[3]);
      out->stat.boilerOut = toFloat(f[4]);
      out->stat.pumpOut = toFloat(f[5]);
      out->stat.tempSet = toFloat(f[6]);
      out->stat.pressSet = toFloat(f[7]);
      out->stat.pumpPct = toFloat(f[8]);
      out->stat.linkOk = toInt(f[9]);
      out->stat.faults = toU32(f[10]);
      out->stat.counter = toU32(f[11]);
      break;
    case GP_CMD:
      out->cmd.mode = toInt(f[0]);
      out->cmd.tempSet = toFloat(f[1]);
      out->cmd.pressSet = toFloat(f[2]);
      out->cmd.pumpPct = toFloat(f[3]);
      break;
    case GP_TUNE:
      out->tune.bbRange = toFloat(f[0]);
      out->tune.pidCycle = toFloat(f[1]);
      out->tune.kp = toFloat(f[2]);
      out->tune.ki = toFloat(f[3]);
      out->tune.kd = toFloat(f[4]);
      out->tune.pumpStepUp = toFloat(f[5]);
      out->tune.pumpKp = toFloat(f[6]);
      out->tune.pumpKi = toFloat(f[7]);
      out->tune.pumpKd = toFloat(f[8]);
      break;
    default:
      return GP_ERR_TYPE;
  }
  out->type = type;
  return GP_OK;
}

// ---------------------------------------------------------------- line reader

bool GpLineReader::push(char c) {
  if (ready_) {  // the previous line has been consumed; start a new one
    len_ = 0;
    buf_[0] = '\0';
    ready_ = false;
  }
  if (c == '\r') return false;
  if (c == '\n') {
    if (discard_ || len_ == 0) {
      discard_ = false;
      len_ = 0;
      return false;
    }
    buf_[len_] = '\0';
    ready_ = true;
    return true;
  }
  if (discard_) return false;
  if (len_ >= sizeof(buf_) - 1) {
    overflows_++;
    discard_ = true;
    len_ = 0;
    return false;
  }
  buf_[len_++] = c;
  return false;
}
