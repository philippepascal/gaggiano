// Short history of readings for the mini curves on the main screen. Fixed-size
// ring buffers, no heap. C API so lv_buildUI.c can feed the charts.
#pragma once
#include <stdint.h>

#define HISTORY_CAPACITY 150  // pressure: 30 s at 5 Hz; temperature: 150 s at 1 Hz

enum HistoryId { HISTORY_TEMPERATURE = 0, HISTORY_PRESSURE = 1, HISTORY_COUNT = 2 };

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
