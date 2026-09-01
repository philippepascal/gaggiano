// Proves the harness itself works. Kept as a template for new test files.
#include "check.h"

int main() {
  CHECK(1 + 1 == 2);
  CHECK_EQ_STR("abc", "abc");
  CHECK_NEAR(0.1 + 0.2, 0.3, 1e-9);
  return test_summary("test_harness");
}
