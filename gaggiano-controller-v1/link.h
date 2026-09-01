// Serial link to the screen (USART2): status out, commands in.
// Lines: "<type>;<field>;<field>;...;|\n". Assembled byte by byte every loop
// pass (no blocking, no heap). A line with an unknown type or the wrong number
// of fields is counted and ignored: it never changes the operating mode.
#pragma once
#include <Arduino.h>

extern uint32_t rxLines;      // accepted
extern uint32_t rxRejected;   // unknown type, wrong field count, malformed
extern uint32_t rxOverflows;  // lines longer than LINE_MAX (dropped)

void linkSetup();
void pollScreenSerial();
void handleLine(char *line);           // parses and applies one line (modified in place)
bool sendStatus(uint32_t now, uint32_t loopCounter);
