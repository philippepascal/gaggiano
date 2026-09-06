// Profile file parsing and formatting (screen), including old files.
#include "check.h"
#include "profile_format.h"
#include <cstring>

static void defaults(GaggiaStateT *s, AdvancedSettingsT *a) {
  std::memset(s, 0, sizeof(*s));
  std::memset(a, 0, sizeof(*a));
  s->boilerSetPoint = 98; s->pressureSetPoint = 8; s->steamSetPoint = 134; s->brew_timer = 30;
  s->steam_shot_s = 0.15f; s->steam_gap_s = 2;
  a->boiler_bb_range = 3; a->boiler_PID_cycle = 1000; a->boiler_PID_KP = 10; a->pump_KP = 1;
  std::strcpy(s->notes, "unchanged");
}

int main() {
  GaggiaStateT s; AdvancedSettingsT a;

  // An actual file written by the firmware before this refactor (6-decimal floats,
  // "cicle" spelling, notes with spaces and a colon).
  const char *old =
      "14.5g  gr:1.04;key,value\n"
      "boilerSetPoint,80.010002\n"
      "pressureSetPoint,7.500000\n"
      "steamSetPoint,135.000000\n"
      "steam_max_pressure,4.000000\n"
      "steam_pump_output_percent,4.000000\n"
      "blooming_pressure,1.500000\n"
      "blooming_fill_time,7.000000\n"
      "blooming_wait_time,8.000000\n"
      "brew_timer,33.000000\n"
      "boiler_bb_range,10.000000\n"
      "boiler_PID_cicle,200.000000\n"
      "boiler_PID_KP,5.000000\n"
      "boiler_PID_KI,0.100000\n"
      "boiler_PID_KD,0.040000\n"
      "pump_max_step_up,0.400000\n"
      "pump_KP,1.000000\n"
      "pump_KI,1.700000\n"
      "pump_KD,0.900000\n"
      "unused1,0.000000\n";
  defaults(&s, &a);
  CHECK(profileParse(old, &s, &a) == 19);
  CHECK_NEAR(s.steam_shot_s, 0.15, 0.001);         // no assist timings in old files: default kept
  CHECK_EQ_STR(s.notes, "14.5g  gr:1.04");
  CHECK_NEAR(s.boilerSetPoint, 80.01, 0.001);
  CHECK_NEAR(s.brew_timer, 33, 0.001);
  CHECK_NEAR(a.boiler_PID_cycle, 200, 0.001);   // from the "cicle" key
  CHECK_NEAR(a.boiler_PID_KD, 0.04, 0.0001);
  CHECK_NEAR(a.pump_KD, 0.9, 0.0001);

  // Truncated file (B10): the missing keys keep their current values.
  const char *partial = "key,value\nboilerSetPoint,90\nbrew_timer,25\n";
  defaults(&s, &a);
  CHECK(profileParse(partial, &s, &a) == 2);
  CHECK_EQ_STR(s.notes, "");                    // no notes in this file
  CHECK_NEAR(s.boilerSetPoint, 90, 0.001);
  CHECK_NEAR(s.brew_timer, 25, 0.001);
  CHECK_NEAR(s.pressureSetPoint, 8, 0.001);     // untouched default
  CHECK_NEAR(a.boiler_PID_KP, 10, 0.001);

  // Garbage and unknown keys.
  defaults(&s, &a);
  CHECK(profileParse("this is not a profile", &s, &a) == -1);
  CHECK(profileParse("", &s, &a) == -1);
  CHECK(profileParse("n;key,value\nbogus,1\nboilerSetPoint,91\n", &s, &a) == 1);
  CHECK_NEAR(s.boilerSetPoint, 91, 0.001);
  CHECK(profileParse("n;key,value", &s, &a) == 0);  // header only, no newline

  // Notes longer than the buffer are truncated, not overflowed.
  {
    char big[600];
    std::memset(big, 'n', 400); big[400] = '\0';
    std::strcat(big, ";key,value\nbrew_timer,5\n");
    defaults(&s, &a);
    CHECK(profileParse(big, &s, &a) == 1);
    CHECK(std::strlen(s.notes) == NOTES_MAX - 1);
  }

  // Round trip through the formatter; ';' in notes is neutralised.
  defaults(&s, &a);
  std::strcpy(s.notes, "18g; 36 out");
  s.blooming_pressure = 1.5f; a.boiler_PID_KD = 0.04;
  char text[PROFILE_TEXT_MAX];
  int len = profileFormat(text, sizeof(text), &s, &a);
  CHECK(len > 0);
  CHECK(std::strncmp(text, "18g  36 out;key,value\n", 22) == 0);
  CHECK(std::strstr(text, "boilerSetPoint,98.00\n") != nullptr);
  CHECK(std::strstr(text, "boiler_PID_KD,0.040\n") != nullptr);
  CHECK(std::strstr(text, "steam_shot_s,0.15\nsteam_gap_s,2.00\n") != nullptr);
  CHECK(std::strstr(text, "boiler_PID_cycle,1000.000\n") != nullptr);
  GaggiaStateT s2; AdvancedSettingsT a2;
  defaults(&s2, &a2);
  CHECK(profileParse(text, &s2, &a2) == 21);
  CHECK_NEAR(s2.steam_gap_s, 2, 0.001);
  CHECK_EQ_STR(s2.notes, "18g  36 out");
  CHECK_NEAR(s2.blooming_pressure, 1.5, 0.001);
  CHECK_NEAR(a2.boiler_PID_KD, 0.04, 0.0001);
  // too small a buffer
  char tiny[16];
  CHECK(profileFormat(tiny, sizeof(tiny), &s, &a) == -1);
  CHECK(tiny[0] == '\0');

  return test_summary("test_profile");
}
