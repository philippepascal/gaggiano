// Profile file format: "<notes>;key,value\n" followed by one "key,value" line per
// setting. Pure functions, no Arduino, no heap; tested on the host.
#pragma once
#include <stddef.h>
#include "gaggia_state.h"

#define PROFILE_TEXT_MAX 768  // largest profile file we read or write

// Applies the keys found in text to state/adv. Unknown keys are ignored, missing
// keys leave the current values untouched. Returns the number of keys applied,
// or -1 if text is not a profile at all (no header line).
int profileParse(const char *text, GaggiaStateT *state, AdvancedSettingsT *adv);

// Writes the profile text for state/adv into out. Returns the length, or -1 if
// out is too small. Notes are written without ';' or newlines.
int profileFormat(char *out, size_t size, const GaggiaStateT *state, const AdvancedSettingsT *adv);
