// Pins, periods and versions for the ESP32-S3 display (Sunton ESP32-8048S043).
#pragma once

#define SCREEN_FIRMWARE_VERSION "screen-2026-09-01-r3"  // sent in HELLO: no commas, max 31 chars

// --- controller link (UART2)
#define CONTROLLER_RX_PIN 18
#define CONTROLLER_TX_PIN 17
#define CONTROLLER_BAUD 115200
#define CONTROLLER_RX_BUFFER 512  // several STAT lines can queue while the UI is busy

// --- over-the-air updates (ArduinoOTA); the password can be overridden in NVS ("otapass")
#define OTA_DEFAULT_PASSWORD "gaggiano"

// --- USB console
#define CONSOLE_BAUD 115200
#define HEAP_REPORT_MS 10000

// --- display
#define TFT_BL 2  // backlight
#define UI_REFRESH_MS 200  // readings shown 5 times a second; widgets run every pass
#define SPLASH_MS 5000

// --- SD card on SPI (the board's TF slot), FAT formatted
#define SD_sck 12
#define SD_miso 13
#define SD_mosi 11
#define SD_cs 10
#define LOG_FLUSH_MS 1000
