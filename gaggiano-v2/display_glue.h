// Panel (Arduino_GFX RGB), LVGL display and input drivers, touch, boot splash.
#pragma once

bool displaySetup();   // false if the LVGL draw buffer could not be allocated
void displaySplash();  // boot image from the SD card, blocks for SPLASH_MS
