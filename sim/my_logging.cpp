// Host shim: LVGL log lines go to stdout.
#include <cstdio>
extern "C" {
#include "my_logging.h"
}
void my_log(const char *msg) { std::printf("%s\n", msg); }
