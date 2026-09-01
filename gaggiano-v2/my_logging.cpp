#include <Arduino.h>
extern "C" {
  #include "my_logging.h"
}

extern bool debugLog;  // console LOG ON|OFF

void my_log(const char *msg) {
  if (!debugLog) return;
  Serial.println(msg);
}