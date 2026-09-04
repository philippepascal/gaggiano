// Serial link to the screen (USART2), protocol v3: see docs/PROTOCOL.md and
// libraries/GaggiaProtocol. STAT out every 200 ms, HELLO at boot; CMD, TUNE and
// HELLO in. Lines are assembled byte by byte every loop pass (no blocking, no
// heap). A rejected line is counted and changes nothing.
#pragma once
#include <Arduino.h>

extern uint32_t rxLines;      // accepted
extern uint32_t rxRejected;   // bad frame, checksum, type or field count
extern uint32_t rxOverflows;  // lines longer than the limit (dropped whole)
extern bool linkOk;           // a valid CMD arrived within LINK_TIMEOUT_MS

void linkSetup();
void pollScreenSerial();
void handleLine(const char *line, size_t len);  // parses and applies one line
void checkLinkTimeout(uint32_t now);
bool sendStatus(uint32_t now, uint32_t loopCounter, uint32_t maxLoopMsSinceLast);  // true when a STAT went out
