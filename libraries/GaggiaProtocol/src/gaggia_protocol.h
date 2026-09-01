// Gaggiano screen/controller protocol v2. See docs/PROTOCOL.md.
// Plain C++ (no Arduino, no heap) so both firmwares and the host tests share it.
#pragma once
#include <stddef.h>
#include <stdint.h>

#define GP_PROTOCOL_VERSION 2
#define GP_LINE_MAX 120       // bytes per line including the trailing newline
#define GP_FIRMWARE_MAX 32    // firmware string in HELLO, including the terminator

enum GpType { GP_UNKNOWN = 0, GP_HELLO, GP_STAT, GP_CMD, GP_TUNE };

enum GpMode { GP_MODE_OFF = 0, GP_MODE_BREW = 1, GP_MODE_STEAM = 2, GP_MODE_CLEAN = 3 };

struct GpHello {
  int version;
  char firmware[GP_FIRMWARE_MAX];
};

struct GpStat {
  int mode;
  float temp;
  float pressure;
  int valve;
  float boilerOut;
  float pumpOut;
  float tempSet;
  float pressSet;
  float pumpPct;
  int linkOk;
  uint32_t faults;
  uint32_t counter;
};

struct GpCmd {
  int mode;
  float tempSet;
  float pressSet;
  float pumpPct;
};

struct GpTune {
  float bbRange;
  float pidCycle;
  float kp;
  float ki;
  float kd;
  float pumpStepUp;
  float pumpKp;
  float pumpKi;
  float pumpKd;
};

struct GpMessage {
  GpType type;
  union {
    GpHello hello;
    GpStat stat;
    GpCmd cmd;
    GpTune tune;
  };
};

enum GpResult {
  GP_OK = 0,
  GP_ERR_FRAME,     // no '$' or no '*' / bad checksum digits
  GP_ERR_CHECKSUM,  // checksum mismatch
  GP_ERR_TYPE,      // unknown type word
  GP_ERR_FIELDS,    // wrong number of fields for the type
  GP_ERR_LENGTH     // line longer than GP_LINE_MAX or buffer too small
};

// Formats m as "$TYPE,...*HH\n" into buf. Returns the length written (without the
// terminating NUL), or -1 if buf is too small (buf gets an empty string).
int gp_encode(const GpMessage *m, char *buf, size_t bufSize);

// Parses one line (with or without a trailing "\r\n"). Bytes before '$' are ignored.
GpResult gp_decode(const char *line, size_t len, GpMessage *out);

// XOR of the bytes in payload[0..len).
uint8_t gp_checksum(const char *payload, size_t len);

const char *gp_type_name(GpType t);
const char *gp_result_name(GpResult r);

// Assembles lines from a byte stream without blocking. Feed every received byte to
// push(); when it returns true, line() holds a complete NUL-terminated line (no
// newline). Lines longer than GP_LINE_MAX - 1 are dropped whole and counted.
class GpLineReader {
 public:
  GpLineReader() : len_(0), discard_(false), ready_(false), overflows_(0) { buf_[0] = '\0'; }
  bool push(char c);
  const char *line() const { return buf_; }
  size_t length() const { return len_; }
  uint32_t overflows() const { return overflows_; }

 private:
  char buf_[GP_LINE_MAX];
  size_t len_;
  bool discard_;
  bool ready_;  // line() holds a complete line; the next push() starts a new one
  uint32_t overflows_;
};
