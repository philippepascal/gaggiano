// Ring buffers behind the main-screen curves.
#include "check.h"
#include "history.h"

int main() {
  history_reset(HISTORY_PRESSURE);
  CHECK(history_count(HISTORY_PRESSURE) == 0);
  CHECK_NEAR(history_at(HISTORY_PRESSURE, 0), 0, 1e-6);   // empty: safe
  history_push(HISTORY_PRESSURE, 1.5f);
  history_push(HISTORY_PRESSURE, 2.5f);
  CHECK(history_count(HISTORY_PRESSURE) == 2);
  CHECK_NEAR(history_at(HISTORY_PRESSURE, 0), 1.5, 1e-6);
  CHECK_NEAR(history_at(HISTORY_PRESSURE, 1), 2.5, 1e-6);
  CHECK_NEAR(history_at(HISTORY_PRESSURE, 2), 0, 1e-6);   // out of range: safe
  // wrap-around keeps the newest HISTORY_CAPACITY values in order
  history_reset(HISTORY_TEMPERATURE);
  for (int i = 0; i < HISTORY_CAPACITY + 10; i++) history_push(HISTORY_TEMPERATURE, (float)i);
  CHECK(history_count(HISTORY_TEMPERATURE) == HISTORY_CAPACITY);
  CHECK_NEAR(history_at(HISTORY_TEMPERATURE, 0), 10, 1e-6);
  CHECK_NEAR(history_at(HISTORY_TEMPERATURE, HISTORY_CAPACITY - 1), HISTORY_CAPACITY + 9, 1e-6);
  // ids are independent and bad ids are ignored
  CHECK(history_count(HISTORY_PRESSURE) == 2);
  history_push(99, 1);
  CHECK(history_count(99) == 0);
  return test_summary("test_history");
}
