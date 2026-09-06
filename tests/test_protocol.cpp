// Round trips, framing and error handling of the shared protocol library.
#include "check.h"
#include "gaggia_protocol.h"
#include <cstring>

static GpMessage roundtrip(const GpMessage &in, GpResult *res, char *line = nullptr) {
  char buf[GP_LINE_MAX];
  int n = gp_encode(&in, buf, sizeof(buf));
  CHECK(n > 0);
  CHECK(n < GP_LINE_MAX);
  CHECK(buf[0] == '$');
  CHECK(buf[n - 1] == '\n');
  if (line) std::strcpy(line, buf);
  GpMessage out;
  std::memset(&out, 0, sizeof(out));
  *res = gp_decode(buf, (size_t)n, &out);
  return out;
}

int main() {
  GpResult r;

  // --- HELLO
  GpMessage h;
  h.type = GP_HELLO;
  h.hello.version = GP_PROTOCOL_VERSION;
  std::strcpy(h.hello.firmware, "controller-2026-09-01");
  GpMessage ho = roundtrip(h, &r);
  CHECK(r == GP_OK);
  CHECK(ho.type == GP_HELLO);
  CHECK(ho.hello.version == 4);
  CHECK_EQ_STR(ho.hello.firmware, "controller-2026-09-01");

  // --- STAT
  GpMessage s;
  s.type = GP_STAT;
  s.stat.mode = GP_MODE_BREW; s.stat.temp = 92.85f; s.stat.pressure = 8.97f; s.stat.valve = 1;
  s.stat.boilerOut = 34.0f; s.stat.pumpOut = 88.4f; s.stat.tempSet = 93.0f; s.stat.pressSet = 9.0f;
  s.stat.pumpPct = 0; s.stat.linkOk = 1; s.stat.faults = 3; s.stat.counter = 4000000000u;
  s.stat.pressStale = 1; s.stat.i2cRecoveries = 7; s.stat.maxLoopMs = 12;
  char sline[GP_LINE_MAX];
  GpMessage so = roundtrip(s, &r, sline);
  CHECK(r == GP_OK);
  CHECK(so.type == GP_STAT);
  CHECK(so.stat.mode == 1);
  CHECK_NEAR(so.stat.temp, 92.85, 0.005);
  CHECK_NEAR(so.stat.pressure, 8.97, 0.005);
  CHECK(so.stat.valve == 1);
  CHECK_NEAR(so.stat.boilerOut, 34.0, 0.05);
  CHECK_NEAR(so.stat.pumpOut, 88.4, 0.005);
  CHECK_NEAR(so.stat.tempSet, 93.0, 0.005);
  CHECK_NEAR(so.stat.pressSet, 9.0, 0.005);
  CHECK(so.stat.linkOk == 1);
  CHECK(so.stat.faults == 3);
  CHECK(so.stat.counter == 4000000000u);
  CHECK(so.stat.pressStale == 1);
  CHECK(so.stat.i2cRecoveries == 7);
  CHECK(so.stat.maxLoopMs == 12);
  CHECK(std::strncmp(sline, "$STAT,1,92.85,8.97,1,34.0,88.40,93.00,9.00,0.00,1,3,4000000000,1,7,12*", 69) == 0);
  // a v2 STAT (12 fields) is rejected on field count, never half-applied
  CHECK(gp_decode("$STAT,1,92.85,8.97,1,34.0,88.40,93.00,9.00,0.00,1,3,4000000000*3A", 66, &so) != GP_OK);

  // --- CMD
  GpMessage c;
  c.type = GP_CMD;
  c.cmd.mode = GP_MODE_STEAM; c.cmd.tempSet = 135; c.cmd.pressSet = 4; c.cmd.pumpPct = 4;
  char cline[GP_LINE_MAX];
  GpMessage co = roundtrip(c, &r, cline);
  CHECK(r == GP_OK);
  CHECK(co.type == GP_CMD);
  CHECK(co.cmd.mode == 2);
  CHECK_NEAR(co.cmd.tempSet, 135, 0.005);
  CHECK_NEAR(co.cmd.pressSet, 4, 0.005);
  CHECK_NEAR(co.cmd.pumpPct, 4, 0.005);
  CHECK_EQ_STR(cline, "$CMD,2,135.00,4.00,4.00*61\n");  // 0x61 verified independently (python XOR)

  // --- TUNE
  GpMessage t;
  t.type = GP_TUNE;
  t.tune.bbRange = 10; t.tune.pidCycle = 200; t.tune.kp = 5; t.tune.ki = 0.1f; t.tune.kd = 0.04f;
  t.tune.pumpStepUp = 0.4f; t.tune.pumpKp = 1; t.tune.pumpKi = 1.7f; t.tune.pumpKd = 0.9f;
  t.tune.steamShotS = 0.15f; t.tune.steamGapS = 2;
  GpMessage to = roundtrip(t, &r);
  CHECK(r == GP_OK);
  CHECK(to.type == GP_TUNE);
  CHECK_NEAR(to.tune.kd, 0.04, 0.0005);
  CHECK_NEAR(to.tune.pumpKi, 1.7, 0.0005);
  CHECK_NEAR(to.tune.steamShotS, 0.15, 0.0005);
  CHECK_NEAR(to.tune.steamGapS, 2, 0.0005);
  // a v3 TUNE (9 fields) is rejected on field count, never half-applied
  {
    const char *l = "$TUNE,10.00,200.00,5.000,0.100,0.040,0.400,1.000,1.700,0.900*";
    char buf[GP_LINE_MAX];
    size_t n = std::strlen(l);
    std::memcpy(buf, l, n);
    n += (size_t)std::snprintf(buf + n, sizeof(buf) - n, "%02X\n", gp_checksum(l + 1, n - 2));
    GpMessage m;
    CHECK(gp_decode(buf, n, &m) == GP_ERR_FIELDS);
  }

  // --- checksum by hand on a tiny payload
  CHECK(gp_checksum("A", 1) == 'A');
  CHECK(gp_checksum("AB", 2) == ('A' ^ 'B'));
  {
    const char *payload = "CMD,2,135.00,4.00,4.00";
    char expect[8];
    std::snprintf(expect, sizeof(expect), "*%02X\n", gp_checksum(payload, std::strlen(payload)));
    CHECK(std::strstr(cline, expect) != nullptr);
  }

  // --- error handling
  GpMessage m;
  CHECK(gp_decode("garbage", 7, &m) == GP_ERR_FRAME);
  CHECK(gp_decode("$CMD,1,0,0,0", 12, &m) == GP_ERR_FRAME);        // no checksum
  CHECK(gp_decode("$CMD,1,0,0,0*ZZ", 15, &m) == GP_ERR_FRAME);     // bad hex
  {
    char bad[GP_LINE_MAX];
    std::strcpy(bad, cline);
    bad[6] = '9';                                                   // flip a digit
    CHECK(gp_decode(bad, std::strlen(bad), &m) == GP_ERR_CHECKSUM);
    CHECK(m.type == GP_UNKNOWN);
  }
  {
    const char *p = "NOPE,1"; char l[32];
    std::snprintf(l, sizeof(l), "$%s*%02X", p, gp_checksum(p, std::strlen(p)));
    CHECK(gp_decode(l, std::strlen(l), &m) == GP_ERR_TYPE);
  }
  {
    const char *p = "CMD,1,0"; char l[32];                          // too few fields
    std::snprintf(l, sizeof(l), "$%s*%02X", p, gp_checksum(p, std::strlen(p)));
    CHECK(gp_decode(l, std::strlen(l), &m) == GP_ERR_FIELDS);
  }
  {
    const char *p = "CMD,1,0,0,0,0"; char l[32];                    // too many fields
    std::snprintf(l, sizeof(l), "$%s*%02X", p, gp_checksum(p, std::strlen(p)));
    CHECK(gp_decode(l, std::strlen(l), &m) == GP_ERR_FIELDS);
  }
  {
    char l[GP_LINE_MAX + 40];
    std::memset(l, 'x', sizeof(l));
    CHECK(gp_decode(l, sizeof(l), &m) == GP_ERR_LENGTH);
  }
  {
    // boot noise before '$', and CRLF after: still decodes
    char l[GP_LINE_MAX + 64];  // room for the prefix, the line, and the CRLF appended below
    std::snprintf(l, sizeof(l), "\x1b[0m junk %s", cline);
    size_t n = std::strlen(l);
    l[n - 1] = '\r'; l[n] = '\n'; l[n + 1] = '\0';
    CHECK(gp_decode(l, n + 1, &m) == GP_OK);
    CHECK(m.type == GP_CMD);
  }
  // encode into a too-small buffer
  {
    char small[10];
    CHECK(gp_encode(&s, small, sizeof(small)) == -1);
    CHECK(small[0] == '\0');
  }

  // --- line reader
  {
    GpLineReader rd;
    const char *stream = "$A*00\r\n\n$B*00\n";
    int lines = 0;
    for (const char *p = stream; *p; p++) {
      if (rd.push(*p)) {
        lines++;
        if (lines == 1) CHECK_EQ_STR(rd.line(), "$A*00");
        if (lines == 2) CHECK_EQ_STR(rd.line(), "$B*00");
      }
    }
    CHECK(lines == 2);
    CHECK(rd.overflows() == 0);
  }
  {
    GpLineReader rd;
    int lines = 0;
    for (int i = 0; i < GP_LINE_MAX + 50; i++) CHECK(!rd.push('y'));   // oversize line
    if (rd.push('\n')) lines++;                                         // dropped whole
    CHECK(lines == 0);
    CHECK(rd.overflows() == 1);
    const char *ok = "$C*00\n";
    for (const char *p = ok; *p; p++) if (rd.push(*p)) lines++;          // reader recovers
    CHECK(lines == 1);
    CHECK_EQ_STR(rd.line(), "$C*00");
  }
  // --- reader + decoder end to end with two messages in one buffer
  {
    GpLineReader rd;
    char two[2 * GP_LINE_MAX + 8];
    std::snprintf(two, sizeof(two), "%s%s", cline, sline);
    int got = 0;
    for (const char *p = two; *p; p++) {
      if (rd.push(*p)) {
        GpMessage mm;
        CHECK(gp_decode(rd.line(), rd.length(), &mm) == GP_OK);
        got++;
        if (got == 1) CHECK(mm.type == GP_CMD);
        if (got == 2) CHECK(mm.type == GP_STAT);
      }
    }
    CHECK(got == 2);
  }
  return test_summary("test_protocol");
}
