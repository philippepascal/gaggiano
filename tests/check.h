// Minimal test helper: CHECK(cond) records a failure and keeps going; the test
// binary's exit status is the number of failures (0 = pass). No dependencies.
#pragma once
#include <cstdio>
#include <cstring>

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                         \
  do {                                                                      \
    g_checks++;                                                             \
    if (!(cond)) {                                                          \
      g_failures++;                                                         \
      std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
    }                                                                       \
  } while (0)

#define CHECK_EQ_STR(a, b)                                                  \
  do {                                                                      \
    g_checks++;                                                             \
    if (std::strcmp((a), (b)) != 0) {                                       \
      g_failures++;                                                         \
      std::printf("  FAIL %s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
    }                                                                       \
  } while (0)

#define CHECK_NEAR(a, b, eps)                                               \
  do {                                                                      \
    g_checks++;                                                             \
    double _d = (double)(a) - (double)(b);                                  \
    if (_d < -(eps) || _d > (eps)) {                                        \
      g_failures++;                                                         \
      std::printf("  FAIL %s:%d: %g != %g\n", __FILE__, __LINE__, (double)(a), (double)(b)); \
    }                                                                       \
  } while (0)

// Call at the end of main(): prints a summary, returns the exit status.
static int test_summary(const char* name) {
  std::printf("%s: %d checks, %d failures\n", name, g_checks, g_failures);
  return g_failures;
}
