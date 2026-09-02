#include "history.h"

struct Ring {
  float v[HISTORY_CAPACITY];
  int head;   // next write position
  int count;
};
static Ring rings[HISTORY_COUNT];

void history_reset(int id) {
  if (id < 0 || id >= HISTORY_COUNT) return;
  rings[id].head = 0;
  rings[id].count = 0;
}

void history_push(int id, float value) {
  if (id < 0 || id >= HISTORY_COUNT) return;
  Ring &r = rings[id];
  r.v[r.head] = value;
  r.head = (r.head + 1) % HISTORY_CAPACITY;
  if (r.count < HISTORY_CAPACITY) r.count++;
}

int history_count(int id) {
  if (id < 0 || id >= HISTORY_COUNT) return 0;
  return rings[id].count;
}

float history_at(int id, int index) {
  if (id < 0 || id >= HISTORY_COUNT) return 0;
  const Ring &r = rings[id];
  if (index < 0 || index >= r.count) return 0;
  int oldest = (r.head - r.count + HISTORY_CAPACITY) % HISTORY_CAPACITY;
  return r.v[(oldest + index) % HISTORY_CAPACITY];
}
