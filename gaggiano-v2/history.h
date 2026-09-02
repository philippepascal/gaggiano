// Short history of readings for the mini curves on the main screen. Fixed-size
// ring buffers, no heap. C API so lv_buildUI.c can feed the charts.
#pragma once
#include <stdint.h>

#define HISTORY_CAPACITY 300  // tiles: 150 points used; graph: 300 points at 2 Hz = 150 s

enum HistoryId {
  HISTORY_TEMPERATURE = 0,  // tile, 1 Hz
  HISTORY_PRESSURE = 1,     // tile, 5 Hz
  HISTORY_G_TEMP = 2,       // graph series, 2 Hz
  HISTORY_G_PRESS = 3,
  HISTORY_G_BOILER = 4,
  HISTORY_G_PUMP = 5,
  HISTORY_G_MODE = 6,
  HISTORY_COUNT = 7
};

#ifdef __cplusplus
extern "C" {
#endif
void history_reset(int id);
void history_push(int id, float value);
int history_count(int id);            // 0..HISTORY_CAPACITY
float history_at(int id, int index);  // 0 = oldest kept, count-1 = newest
#ifdef __cplusplus
}
#endif
