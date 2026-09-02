#include "check.h"
#include "timezones.h"
#include <cstring>

int main() {
  CHECK(timezoneCount() == 12);
  CHECK(timezoneDefault() == 0);
  CHECK_EQ_STR(timezoneName(0), "Los Angeles");
  CHECK_EQ_STR(timezonePosix(0), "PST8PDT,M3.2.0,M11.1.0");
  CHECK_EQ_STR(timezoneName(-1), "");
  CHECK_EQ_STR(timezonePosix(99), "UTC0");
  // every POSIX string starts with a name and carries an offset digit
  for (int i = 0; i < timezoneCount(); i++) {
    const char *p = timezonePosix(i);
    CHECK(std::strlen(p) >= 4);
    CHECK(std::strpbrk(p, "0123456789") != nullptr);
  }
  const char *opts = timezoneOptions();
  int newlines = 0;
  for (const char *c = opts; *c; c++) if (*c == '\n') newlines++;
  CHECK(newlines == timezoneCount() - 1);
  CHECK(std::strstr(opts, "Paris\nAthens") != nullptr);
  return test_summary("test_timezones");
}
