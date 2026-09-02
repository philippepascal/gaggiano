#include "theme.h"

lv_color_t theme_bg(void) { return lv_color_hex(0x0F1113); }
lv_color_t theme_surface(void) { return lv_color_hex(0x171A1D); }
lv_color_t theme_steel(void) { return lv_color_hex(0x3C434A); }
lv_color_t theme_text(void) { return lv_color_hex(0xE9ECEF); }
lv_color_t theme_muted(void) { return lv_color_hex(0x8B949E); }
lv_color_t theme_amber(void) { return lv_color_hex(0xD9A441); }
lv_color_t theme_steam(void) { return lv_color_hex(0x7FB3D5); }
static lv_color_t amber_ink(void) { return lv_color_hex(0x1A1408); }
static lv_color_t dim_text(void) { return lv_color_hex(0x4A5158); }
static lv_color_t dim_border(void) { return lv_color_hex(0x262B30); }

const lv_font_t *theme_font_small = &lv_font_montserrat_14;
const lv_font_t *theme_font_name = &lv_font_montserrat_20;
const lv_font_t *theme_font_value = &lv_font_montserrat_28;
const lv_font_t *theme_font_big = &lv_font_montserrat_48;

static lv_style_t st_screen, st_header, st_surface, st_muted, st_title;
static lv_style_t st_btn, st_btn_on, st_btn_on_steam, st_btn_dis;

void theme_init(void) {
  // The default theme (dark) is the base for widgets we do not style by hand.
  lv_theme_default_init(NULL, theme_amber(), theme_steel(), true, theme_font_small);

  lv_style_init(&st_screen);
  lv_style_set_bg_color(&st_screen, theme_bg());
  lv_style_set_bg_opa(&st_screen, LV_OPA_COVER);
  lv_style_set_text_color(&st_screen, theme_text());
  lv_style_set_text_font(&st_screen, theme_font_small);

  lv_style_init(&st_header);
  lv_style_set_bg_color(&st_header, theme_bg());
  lv_style_set_bg_opa(&st_header, LV_OPA_COVER);
  lv_style_set_border_color(&st_header, theme_steel());
  lv_style_set_border_width(&st_header, 1);
  lv_style_set_border_side(&st_header, LV_BORDER_SIDE_BOTTOM);
  lv_style_set_radius(&st_header, 0);
  lv_style_set_text_color(&st_header, theme_muted());

  lv_style_init(&st_surface);
  lv_style_set_bg_color(&st_surface, theme_surface());
  lv_style_set_bg_opa(&st_surface, LV_OPA_COVER);
  lv_style_set_border_width(&st_surface, 0);
  lv_style_set_radius(&st_surface, 6);
  lv_style_set_shadow_width(&st_surface, 0);

  lv_style_init(&st_muted);
  lv_style_set_text_color(&st_muted, theme_muted());
  lv_style_set_text_font(&st_muted, theme_font_small);

  lv_style_init(&st_title);
  lv_style_set_text_color(&st_title, theme_text());
  lv_style_set_text_font(&st_title, theme_font_name);

  lv_style_init(&st_btn);
  lv_style_set_bg_color(&st_btn, theme_surface());
  lv_style_set_bg_opa(&st_btn, LV_OPA_COVER);
  lv_style_set_border_color(&st_btn, theme_steel());
  lv_style_set_border_width(&st_btn, 1);
  lv_style_set_radius(&st_btn, 8);
  lv_style_set_shadow_width(&st_btn, 0);
  lv_style_set_text_color(&st_btn, theme_text());
  lv_style_set_text_font(&st_btn, theme_font_name);

  lv_style_init(&st_btn_on);
  lv_style_set_bg_color(&st_btn_on, theme_amber());
  lv_style_set_border_color(&st_btn_on, theme_amber());
  lv_style_set_text_color(&st_btn_on, amber_ink());

  lv_style_init(&st_btn_on_steam);
  lv_style_set_bg_color(&st_btn_on_steam, theme_steam());
  lv_style_set_border_color(&st_btn_on_steam, theme_steam());
  lv_style_set_text_color(&st_btn_on_steam, amber_ink());

  lv_style_init(&st_btn_dis);
  lv_style_set_bg_color(&st_btn_dis, theme_surface());
  lv_style_set_border_color(&st_btn_dis, dim_border());
  lv_style_set_text_color(&st_btn_dis, dim_text());
}

void theme_apply_screen(lv_obj_t *scr) { lv_obj_add_style(scr, &st_screen, 0); }
void theme_apply_header(lv_obj_t *obj) { lv_obj_add_style(obj, &st_header, 0); }
void theme_apply_surface(lv_obj_t *obj) { lv_obj_add_style(obj, &st_surface, 0); }
void theme_apply_muted(lv_obj_t *label) { lv_obj_add_style(label, &st_muted, 0); }
void theme_apply_title(lv_obj_t *label) { lv_obj_add_style(label, &st_title, 0); }

void theme_apply_btn(lv_obj_t *btn, bool steam) {
  lv_obj_add_style(btn, &st_btn, 0);
  lv_obj_add_style(btn, steam ? &st_btn_on_steam : &st_btn_on, LV_STATE_CHECKED);
  lv_obj_add_style(btn, &st_btn_dis, LV_STATE_DISABLED);
}
