// Over-the-air firmware update of the screen (ArduinoOTA on port 3232).
// ./gg flash screen --ota sends a build; the panel shows the progress.
#pragma once
#include <stdint.h>

void otaBegin();
void otaPoll(uint32_t now);
