#include "display_glue.h"
#include "config.h"
#include "storage.h"
#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>

// Sunton ESP32-8048S043: ST7262 IPS 800x480 over the S3's RGB peripheral.
// Pins and timings as in Arduino_GFX's own declaration of this board.
static Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
    0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 8 /* hsync_back_porch */,
    0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 8 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 14000000 /* prefer_speed, as before the core upgrade */, false /* useBigEndian */,
    0 /* de_idle_high */, 0 /* pclk_idle_high */, 800 * PANEL_BOUNCE_LINES /* bounce_buffer_size_px */);

static Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */);

// GT911 capacitive touch; pins and mapping are configured in touch.h.
#include "touch.h"

static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
  lv_disp_flush_ready(disp);
}

static void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  (void)indev_driver;
  if (touch_has_signal()) {
    if (touch_touched()) {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    } else if (touch_released()) {
      data->state = LV_INDEV_STATE_REL;
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

bool displaySetup() {
  gfx->begin();
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  lv_init();
  delay(10);
  touch_init();

  screenWidth = gfx->width();
  screenHeight = gfx->height();
  // The draw buffer lives in internal RAM (PSRAM would be too slow for the flush).
  // DRAW_BUFFER_LINES rows: small enough to leave room for WiFi and the panel's bounce buffers.
  size_t px = (size_t)screenWidth * DRAW_BUFFER_LINES;
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * px, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    Serial.println("LVGL draw buffer allocation failed");
    return false;
  }
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, px);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
  return true;
}

static void bmpDrawCallback(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h) {
  gfx->draw16bitRGBBitmap(x, y, bitmap, w, h);
}

void displaySplash() {
  displayFrankBmp(bmpDrawCallback, 800, 480);
  delay(SPLASH_MS);
}
