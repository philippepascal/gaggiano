// Visual identity of the screen: palette, fonts and styles by role. Everything
// that decides how a widget looks lives here, nothing in lv_buildUI.c. Plain C so
// the simulator and the firmware share it. See docs/UI-PLAN.md for the palette.
#pragma once
#include <lvgl.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Palette
lv_color_t theme_bg(void);        // ground
lv_color_t theme_surface(void);   // tiles, idle buttons, header strip
lv_color_t theme_steel(void);     // borders, active tab
lv_color_t theme_text(void);
lv_color_t theme_muted(void);
lv_color_t theme_amber(void);     // anything running
lv_color_t theme_steam(void);     // steaming

// Fonts (Montserrat, built into lvgl; enabled in lv_conf.h)
extern const lv_font_t *theme_font_small;  // 14: labels, menu, notes
extern const lv_font_t *theme_font_name;   // 20: button names, header title
extern const lv_font_t *theme_font_value;  // 28: button values
extern const lv_font_t *theme_font_big;    // 48: tile readings

void theme_init(void);  // call once after lv_init(), before building screens

// Roles
void theme_apply_screen(lv_obj_t *scr);
void theme_apply_header(lv_obj_t *obj);           // header strip (the tab buttons area)
void theme_apply_surface(lv_obj_t *obj);          // plain panel on the surface color
void theme_apply_btn(lv_obj_t *btn, bool steam);  // idle / checked (amber or steam) / disabled
void theme_apply_muted(lv_obj_t *label);          // secondary text
void theme_apply_title(lv_obj_t *label);          // header title (profile name)

#ifdef __cplusplus
}
#endif
