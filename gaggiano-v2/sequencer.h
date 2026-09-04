// Brew sequencer: turns the button state (isBrewing, isBlooming, isAuto, ...) and
// time into the command to send to the controller, including the bloom fill /
// bloom wait / timed brew phases. Pure logic, no Arduino; tested on the host.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "gaggia_state.h"

#define PHASE_OFF 0
#define PHASE_BLOOM_FILL 2
#define PHASE_BLOOM_WAIT 3
#define PHASE_BREW 4

#ifdef __cplusplus
extern "C" {
#endif

struct SeqCommand {
  int mode;  // GP_MODE_* from gaggia_protocol.h
  float tempSet;
  float pressSet;
  float pumpPct;
};

// Advances the phase machine. Clears state->hasCommandChanged when it consumes it.
// Returns true when *out holds a command that must be sent to the controller.
bool sequencerStep(GaggiaStateT *state, uint32_t now, struct SeqCommand *out);
int sequencerPhase(void);
bool sequencerBloomConfigured(const GaggiaStateT *state);  // a prime/auto run will have a bloom fill and wait
void sequencerReset(void);

#ifdef __cplusplus
}
#endif
