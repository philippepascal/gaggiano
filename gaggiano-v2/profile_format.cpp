#include "profile_format.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Key {
  const char *name;
  int which;  // 0: float in state, 1: double in adv
  size_t offset;
};

#define SK(n, f) {n, 0, offsetof(GaggiaState, f)}
#define AK(n, f) {n, 1, offsetof(AdvancedSettings, f)}

static const Key kKeys[] = {
    SK("boilerSetPoint", boilerSetPoint),
    SK("pressureSetPoint", pressureSetPoint),
    SK("steamSetPoint", steamSetPoint),
    SK("steam_max_pressure", steam_max_pressure),
    SK("steam_pump_output_percent", steam_pump_output_percent),
    SK("steam_shot_s", steam_shot_s),
    SK("steam_gap_s", steam_gap_s),
    SK("steam_min_temp", steam_min_temp),
    SK("blooming_pressure", blooming_pressure),
    SK("blooming_fill_time", blooming_fill_time),
    SK("blooming_wait_time", blooming_wait_time),
    SK("brew_timer", brew_timer),
    AK("boiler_bb_range", boiler_bb_range),
    AK("boiler_PID_cycle", boiler_PID_cycle),
    AK("boiler_PID_cicle", boiler_PID_cycle),  // spelling used by files written before 2026-09
    AK("boiler_PID_KP", boiler_PID_KP),
    AK("boiler_PID_KI", boiler_PID_KI),
    AK("boiler_PID_KD", boiler_PID_KD),
    AK("pump_max_step_up", pump_max_step_up),
    AK("pump_KP", pump_KP),
    AK("pump_KI", pump_KI),
    AK("pump_KD", pump_KD),
    AK("pump_flow_ml_s", pump_flow_ml_s),
};
static const size_t kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);

static void apply(const Key &k, double v, GaggiaStateT *state, AdvancedSettingsT *adv) {
  if (k.which == 0) {
    *(float *)((char *)state + k.offset) = (float)v;
  } else {
    *(double *)((char *)adv + k.offset) = v;
  }
}

int profileParse(const char *text, GaggiaStateT *state, AdvancedSettingsT *adv) {
  const char *p = text;
  // First line: optional "<notes>;" then the header "key,value".
  const char *eol = strchr(p, '\n');
  size_t firstLen = eol ? (size_t)(eol - p) : strlen(p);
  const char *semi = (const char *)memchr(p, ';', firstLen);
  const char *header = p;
  size_t headerLen = firstLen;
  if (semi != NULL) {
    size_t n = (size_t)(semi - p);
    if (n >= NOTES_MAX) n = NOTES_MAX - 1;
    memcpy(state->notes, p, n);
    state->notes[n] = '\0';
    header = semi + 1;
    headerLen = firstLen - (size_t)(header - p);
  } else {
    state->notes[0] = '\0';
  }
  if (headerLen < 9 || strncmp(header, "key,value", 9) != 0) return -1;
  if (eol == NULL) return 0;
  p = eol + 1;

  int applied = 0;
  while (*p != '\0') {
    eol = strchr(p, '\n');
    size_t len = eol ? (size_t)(eol - p) : strlen(p);
    const char *comma = (const char *)memchr(p, ',', len);
    if (comma != NULL) {
      size_t klen = (size_t)(comma - p);
      for (size_t i = 0; i < kKeyCount; i++) {
        if (strlen(kKeys[i].name) == klen && strncmp(kKeys[i].name, p, klen) == 0) {
          apply(kKeys[i], strtod(comma + 1, NULL), state, adv);
          applied++;
          break;
        }
      }
    }
    if (eol == NULL) break;
    p = eol + 1;
  }
  return applied;
}

int profileFormat(char *out, size_t size, const GaggiaStateT *state, const AdvancedSettingsT *adv) {
  // Notes: no ';' (the delimiter) and no newlines.
  char notes[NOTES_MAX];
  size_t n = 0;
  for (const char *c = state->notes; *c && n < NOTES_MAX - 1; c++) {
    notes[n++] = (*c == ';' || *c == '\n' || *c == '\r') ? ' ' : *c;
  }
  notes[n] = '\0';
  int len = snprintf(out, size,
                     "%s;key,value\n"
                     "boilerSetPoint,%.2f\n"
                     "pressureSetPoint,%.2f\n"
                     "steamSetPoint,%.2f\n"
                     "steam_max_pressure,%.2f\n"
                     "steam_pump_output_percent,%.2f\n"
                     "steam_shot_s,%.2f\n"
                     "steam_gap_s,%.2f\n"
                     "steam_min_temp,%.1f\n"
                     "blooming_pressure,%.2f\n"
                     "blooming_fill_time,%.2f\n"
                     "blooming_wait_time,%.2f\n"
                     "brew_timer,%.2f\n"
                     "boiler_bb_range,%.3f\n"
                     "boiler_PID_cycle,%.3f\n"
                     "boiler_PID_KP,%.3f\n"
                     "boiler_PID_KI,%.3f\n"
                     "boiler_PID_KD,%.3f\n"
                     "pump_max_step_up,%.3f\n"
                     "pump_KP,%.3f\n"
                     "pump_KI,%.3f\n"
                     "pump_KD,%.3f\n"
                     "pump_flow_ml_s,%.1f\n",
                     notes, (double)state->boilerSetPoint, (double)state->pressureSetPoint,
                     (double)state->steamSetPoint, (double)state->steam_max_pressure,
                     (double)state->steam_pump_output_percent, (double)state->steam_shot_s,
                     (double)state->steam_gap_s, (double)state->steam_min_temp, (double)state->blooming_pressure,
                     (double)state->blooming_fill_time, (double)state->blooming_wait_time,
                     (double)state->brew_timer, adv->boiler_bb_range, adv->boiler_PID_cycle,
                     adv->boiler_PID_KP, adv->boiler_PID_KI, adv->boiler_PID_KD, adv->pump_max_step_up,
                     adv->pump_KP, adv->pump_KI, adv->pump_KD, adv->pump_flow_ml_s);
  if (len < 0 || (size_t)len >= size) {
    if (size) out[0] = '\0';
    return -1;
  }
  return len;
}
