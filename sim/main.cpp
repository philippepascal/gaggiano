// Gaggiano screen simulator: runs the real UI (lv_buildUI.c, theme, sequencer) in an
// SDL2 window on the Mac with a fake controller and in-memory profiles.
//
//   ./gg sim                 interactive window
//   ./gg sim --shot out.bmp  render for a second, save a screenshot, exit
//
// Keys: t/T temperature +/-, p/P pressure +/-, r reset readings, w write shot.bmp,
//       q or Escape quit. The mouse is the touch panel.
#include <SDL.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <lvgl.h>
#include "gaggia_state.h"
#include "lv_buildUI.h"
extern "C" lv_obj_t *heat_btn_for_scene(void);   // simulator hooks exported by lv_buildUI.c
extern "C" lv_obj_t *brew_btn_for_scene(void);
extern "C" lv_obj_t *steam_btn_for_scene(void);
extern "C" lv_obj_t *menu_for_scene(void);
extern "C" void show_view_for_scene(int index);
#include "sequencer.h"
#include <gaggia_protocol.h>

static const int W = 800, H = 480;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static lv_color_t framebuf[W * H];
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static bool mouseDown = false;
static int mouseX = 0, mouseY = 0;

static uint32_t nowMs() {
  using namespace std::chrono;
  static const auto t0 = steady_clock::now();
  return (uint32_t)duration_cast<milliseconds>(steady_clock::now() - t0).count();
}
extern "C" uint32_t millis(void) { return nowMs(); }
static bool headless = false;  // --shot: no window, LVGL renders into framebuf only

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  for (int y = area->y1; y <= area->y2; y++) {
    memcpy(&framebuf[y * W + area->x1], color_p, (area->x2 - area->x1 + 1) * sizeof(lv_color_t));
    color_p += area->x2 - area->x1 + 1;
  }
  lv_disp_flush_ready(drv);
}

static void mouse_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  (void)drv;
  data->point.x = mouseX;
  data->point.y = mouseY;
  data->state = mouseDown ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

static void present() {
  if (headless) return;
  SDL_UpdateTexture(texture, NULL, framebuf, W * sizeof(lv_color_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

// 24-bit BMP from the RGB565 framebuffer; no SDL involved so it works headless.
static void saveShot(const char *path) {
  FILE *f = std::fopen(path, "wb");
  if (!f) { std::printf("screenshot failed: cannot open %s\n", path); return; }
  const uint32_t rowBytes = (W * 3 + 3) & ~3u, dataSize = rowBytes * H, fileSize = 54 + dataSize;
  uint8_t hdr[54] = {'B', 'M'};
  auto put32 = [&](int at, uint32_t v) { for (int i = 0; i < 4; i++) hdr[at + i] = (uint8_t)(v >> (8 * i)); };
  auto put16 = [&](int at, uint16_t v) { hdr[at] = (uint8_t)v; hdr[at + 1] = (uint8_t)(v >> 8); };
  put32(2, fileSize); put32(10, 54); put32(14, 40); put32(18, W); put32(22, H); put16(26, 1); put16(28, 24); put32(34, dataSize);
  std::fwrite(hdr, 1, 54, f);
  std::vector<uint8_t> row(rowBytes, 0);
  for (int y = H - 1; y >= 0; y--) {
    for (int x = 0; x < W; x++) {
      uint16_t px = framebuf[y * W + x].full;
      row[x * 3 + 0] = (uint8_t)((px & 0x1F) << 3);          // blue
      row[x * 3 + 1] = (uint8_t)(((px >> 5) & 0x3F) << 2);   // green
      row[x * 3 + 2] = (uint8_t)(((px >> 11) & 0x1F) << 3);  // red
    }
    std::fwrite(row.data(), 1, rowBytes, f);
  }
  std::fclose(f);
  std::printf("screenshot written to %s\n", path);
}

// ---------------------------------------------------------------- fake state

GaggiaStateT state = {false, 93, 9.0, 135, 4, 4, 1.5, 7, 8, 33, false, "", "", 0, 0, false, 0,
                      false, false, false, false, false, false, false, false, 0, 0};
AdvancedSettingsT advancedSettings = {false, false, 10, 200, 5, 0.1, 0.04, 0.4, 1, 1.7, 0.9, 0};

// The controller as the screen sees it: readings drift toward the last command.
static SeqCommand lastCmd = {GP_MODE_OFF, 0, 0, 0};
static float temp = 24.0f, pressure = 0.0f;

static void fakeController(float dt) {
  float tTarget = lastCmd.tempSet > 0 ? lastCmd.tempSet : 24.0f;
  temp += (tTarget - temp) * dt * (lastCmd.tempSet > 0 ? 0.15f : 0.05f);
  float pTarget = 0;
  if (lastCmd.mode == GP_MODE_BREW || lastCmd.mode == GP_MODE_CLEAN) pTarget = lastCmd.pressSet;
  if (lastCmd.mode == GP_MODE_STEAM) pTarget = lastCmd.pressSet * 0.6f;
  pressure += (pTarget - pressure) * dt * 1.5f;
  state.tempRead = temp;
  state.pressureRead = pressure;
  state.isSolenoidOn = (lastCmd.mode == GP_MODE_BREW || lastCmd.mode == GP_MODE_CLEAN) && lastCmd.pressSet > 0;
}

// ---------------------------------------------------------------- fake storage

static std::vector<std::string> profiles = {"tigerwalk.csv", "lavazzaGold.csv", "decaf.csv"};
static std::string current = "tigerwalk.csv";

static int writeConfigFile() { std::printf("[storage] save %s\n", current.c_str()); return 1; }
static int listProfiles(char *buf, size_t size) {
  std::string all;
  for (auto &p : profiles) all += p + ";";
  strncpy(buf, all.c_str(), size - 1);
  buf[size - 1] = '\0';
  return (int)profiles.size();
}
static int getCurrentProfile(char *buf, size_t size) { strncpy(buf, current.c_str(), size - 1); buf[size - 1] = '\0'; return 0; }
static int writeCurrentProfile(const char *name) { current = name; return 1; }
static int setupAndReadConfigFile() {
  strncpy(state.profile_name, current.c_str(), PROFILE_NAME_MAX - 1);
  strncpy(state.notes, current == "tigerwalk.csv" ? "15 g in, grind 1.04, tamp light" : "", NOTES_MAX - 1);
  state.hasConfigChanged = true;
  advancedSettings.userChanged = true;
  std::printf("[storage] load %s\n", current.c_str());
  return 1;
}
static int renameProfile(const char *newName) {
  for (auto &p : profiles) if (p == current) p = newName;
  current = newName;
  return 1;
}
static bool deleteProfile(const char *name) {
  for (size_t i = 0; i < profiles.size(); i++) if (profiles[i] == name) { profiles.erase(profiles.begin() + i); return true; }
  return false;
}
static int duplicateProfile() { profiles.push_back(current + "-c"); return 1; }

// ---------------------------------------------------------------- main

int main(int argc, char **argv) {
  const char *shot = NULL;
  const char *scene = "idle";  // idle | heating | brewing | steaming | menu | profiles | settings | advanced
  uint32_t shotAfter = 1000;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--shot") == 0 && i + 1 < argc) shot = argv[++i];
    else if (strcmp(argv[i], "--scene") == 0 && i + 1 < argc) scene = argv[++i];
    else if (strcmp(argv[i], "--after") == 0 && i + 1 < argc) shotAfter = (uint32_t)atoi(argv[++i]);
  }
  setvbuf(stdout, NULL, _IONBF, 0);  // progress visible even when piped
  headless = shot != NULL;
  std::printf("[sim] start%s\n", headless ? " (headless)" : "");
  if (!headless) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { std::fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
    window = SDL_CreateWindow("Gaggiano screen", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, W, H);
  }

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, framebuf, NULL, W * H);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = W;
  disp_drv.ver_res = H;
  disp_drv.flush_cb = flush_cb;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.direct_mode = 1;
  lv_disp_drv_register(&disp_drv);
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = mouse_cb;
  lv_indev_drv_register(&indev_drv);

  std::printf("[sim] lvgl ready, building the UI\n");
  instantiateUI(&state, &advancedSettings, writeConfigFile, listProfiles, getCurrentProfile, writeCurrentProfile,
                setupAndReadConfigFile, renameProfile, deleteProfile, duplicateProfile);
  setupAndReadConfigFile();
  std::printf("[sim] UI built, entering the loop (scene %s)\n", scene);
  if (strcmp(scene, "heating") == 0 || strcmp(scene, "brewing") == 0) {
    state.isBoilerOn = true;
    temp = 90.0f;
    lv_obj_add_state(heat_btn_for_scene(), LV_STATE_CHECKED);
  }
  if (strcmp(scene, "brewing") == 0) {
    state.isBrewing = true;
    state.lastBrewTime = 31;
    lv_obj_add_state(brew_btn_for_scene(), LV_STATE_CHECKED);
  }
  if (strcmp(scene, "steaming") == 0) {
    state.isSteaming = true;
    temp = 128.0f;
    lv_obj_add_state(steam_btn_for_scene(), LV_STATE_CHECKED);
  }
  if (strcmp(scene, "menu") == 0) lv_dropdown_open(menu_for_scene());
  if (strcmp(scene, "profiles") == 0) show_view_for_scene(1);
  if (strcmp(scene, "settings") == 0) show_view_for_scene(2);
  if (strcmp(scene, "advanced") == 0) show_view_for_scene(3);
  if (strcmp(scene, "heating") == 0 || strcmp(scene, "brewing") == 0 || strcmp(scene, "steaming") == 0) state.hasCommandChanged = true;

  uint32_t lastUi = 0, lastTick = nowMs(), start = nowMs();
  bool running = true;
  while (running) {
    SDL_Event e;
    while (!headless && SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) running = false;
      else if (e.type == SDL_MOUSEMOTION) { mouseX = e.motion.x; mouseY = e.motion.y; }
      else if (e.type == SDL_MOUSEBUTTONDOWN) { mouseDown = true; mouseX = e.button.x; mouseY = e.button.y; }
      else if (e.type == SDL_MOUSEBUTTONUP) { mouseDown = false; }
      else if (e.type == SDL_KEYDOWN) {
        SDL_Keycode k = e.key.keysym.sym;
        bool shift = (e.key.keysym.mod & KMOD_SHIFT) != 0;
        if (k == SDLK_q || k == SDLK_ESCAPE) running = false;
        else if (k == SDLK_t) temp += shift ? -5 : 5;
        else if (k == SDLK_p) pressure += shift ? -1 : 1;
        else if (k == SDLK_r) { temp = 24; pressure = 0; }
        else if (k == SDLK_w) saveShot("shot.bmp");
      }
    }
    uint32_t now = nowMs();
    float dt = (now - lastTick) / 1000.0f;
    lastTick = now;

    SeqCommand c;
    if (sequencerStep(&state, now, &c)) {
      lastCmd = c;
      std::printf("[controller] mode=%d temp=%.1f press=%.1f pump=%.0f%%\n", c.mode, c.tempSet, c.pressSet, c.pumpPct);
    }
    fakeController(dt);
    if (now - lastUi >= 200) { lastUi = now; updateUI(); }
    lv_timer_handler();
    present();
    if (shot && now - start > shotAfter) { saveShot(shot); running = false; }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  if (!headless) {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
  return 0;
}
