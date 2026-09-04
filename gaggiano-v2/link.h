// Serial link to the controller (UART2), protocol v3: see docs/PROTOCOL.md and
// libraries/GaggiaProtocol. Non-blocking: call linkPoll() every loop pass.
// Sends CMD when it changes and every LINK_HEARTBEAT_MS; re-sends it when two
// consecutive STAT lines disagree with it; sends TUNE on request and after a
// controller HELLO. Updates the readings in the shared state from STAT.
#pragma once
#include <Arduino.h>
#include "gaggia_state.h"

#define LINK_HEARTBEAT_MS 1000
#define LINK_STAT_TIMEOUT_MS 1500  // no STAT for this long: controller considered gone

extern uint32_t linkRxLines, linkRxRejected, linkRxOverflows, linkTxLines;

void linkSetup(HardwareSerial *serial, GaggiaStateT *state, AdvancedSettingsT *adv);
void linkPoll(uint32_t now);
void linkSetCommand(int mode, float tempSet, float pressSet, float pumpPct);
void linkSendTune();
bool linkControllerAlive(uint32_t now);
int linkControllerMode();  // last mode echoed by the controller, -1 if none yet
