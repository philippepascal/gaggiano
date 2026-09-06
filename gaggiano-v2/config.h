// Pins, periods and versions for the ESP32-S3 display (Sunton ESP32-8048S043).
#pragma once

#define SCREEN_FIRMWARE_VERSION "screen-2026-09-06-r1"  // sent in HELLO: no commas, max 31 chars

// --- controller link (UART2)
#define CONTROLLER_RX_PIN 18
#define CONTROLLER_TX_PIN 17
#define CONTROLLER_BAUD 115200
#define CONTROLLER_RX_BUFFER 512  // several STAT lines can queue while the UI is busy

// --- firmware update over HTTP (POST /update); the password can be overridden in NVS ("otapass")
#define OTA_DEFAULT_PASSWORD "gaggiano"

// --- USB console
#define CONSOLE_BAUD 115200
#define HEAP_REPORT_MS 10000

// --- display
#define TFT_BL 2  // backlight
#define DRAW_BUFFER_LINES 48   // LVGL draw buffer height (internal RAM: 800 x lines x 2 bytes)
#define PANEL_BOUNCE_LINES 16  // RGB panel bounce buffers in internal RAM: keeps the picture steady while WiFi runs
#define UI_REFRESH_MS 200  // readings shown 5 times a second; widgets run every pass
#define SPLASH_MS 5000

// --- SD card on SPI (the board's TF slot), FAT formatted
#define SD_sck 12
#define SD_miso 13
#define SD_mosi 11
#define SD_cs 10
#define LOG_FLUSH_MS 1000
#define LOG_SESSIONS_KEPT 10  // dated session logs kept under /gaggia/logs
