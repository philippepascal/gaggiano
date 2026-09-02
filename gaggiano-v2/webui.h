// The screen's web page: status, live graph, log download. See docs/WEB.md.
#pragma once
#include <stdint.h>

void webBegin();              // starts the HTTP server (requests are served once WiFi is up)
void webPoll(uint32_t now);   // serve pending requests; announce gaggiano.local when connected
