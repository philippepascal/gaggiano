#include "theme.h"

lv_color_t theme_bg(void) { return lv_color_hex(0x0F1113); }
lv_color_t theme_surface(void) { return lv_color_hex(0x171A1D); }
lv_color_t theme_steel(void) { return lv_color_hex(0x3C434A); }
lv_color_t theme_text(void) { return lv_color_hex(0xE9ECEF); }
lv_color_t theme_muted(void) { return lv_color_hex(0x8B949E); }
lv_color_t theme_amber(void) { return lv_color_hex(0xD9A441); }
lv_color_t theme_steam(void) { return lv_color_hex(0x7FB3D5); }
lv_color_t theme_heater(void) { return lv_color_hex(0xD9603B); }
lv_color_t theme_pump(void) { return lv_color_hex(0x6BBF8A); }
lv_color_t theme_clean(void) { return lv_color_hex(0x4FA3A0); }
static lv_color_t amber_ink(void) { return lv_color_hex(0x1A1408); }
static lv_color_t dim_text(void) { return lv_color_hex(0x4A5158); }
static lv_color_t dim_border(void) { return lv_color_hex(0x262B30); }

const lv_font_t *theme_font_small = &lv_font_montserrat_14;
const lv_font_t *theme_font_name = &lv_font_montserrat_20;
const lv_font_t *theme_font_value = &lv_font_montserrat_28;
const lv_font_t *theme_font_big = &lv_font_montserrat_48;

static lv_color_t steel_hi(void) { return lv_color_hex(0x6B7480); }

static lv_style_t st_screen, st_header, st_surface, st_muted, st_title, st_header_notes, st_menu;
static lv_style_t st_btn, st_btn_on, st_btn_on_steam, st_btn_dis, st_btn_name, st_btn_value;
static lv_style_t st_tile, st_tile_label, st_tile_value, st_tile_unit, st_chart;
static lv_style_t st_group_title, st_editor, st_editor_value, st_editor_title;
static lv_style_t st_btn_sub, st_list, st_list_row, st_list_row_sel, st_field_label, st_field, st_view;

void theme_init(void) {
  // The default theme (dark) is the base for widgets we do not style by hand.
  lv_theme_default_init(NULL, theme_amber(), theme_steel(), true, theme_font_small);

  lv_style_init(&st_screen);
  lv_style_set_bg_color(&st_screen, lv_color_black());  // the views float on pure black
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
  lv_style_set_text_font(&st_title, theme_font_value);

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

  lv_style_init(&st_header_notes);
  lv_style_set_text_color(&st_header_notes, theme_muted());
  lv_style_set_text_font(&st_header_notes, theme_font_name);

  lv_style_init(&st_menu);
  lv_style_set_bg_color(&st_menu, theme_surface());
  lv_style_set_bg_opa(&st_menu, LV_OPA_COVER);
  lv_style_set_border_color(&st_menu, theme_steel());
  lv_style_set_border_width(&st_menu, 1);
  lv_style_set_radius(&st_menu, 6);
  lv_style_set_shadow_width(&st_menu, 0);
  lv_style_set_text_color(&st_menu, theme_text());
  lv_style_set_text_font(&st_menu, theme_font_name);
  lv_style_set_pad_all(&st_menu, 4);

  lv_style_init(&st_btn_name);
  lv_style_set_text_font(&st_btn_name, theme_font_value);
  lv_style_set_text_opa(&st_btn_name, LV_OPA_70);  // quieter than the value, in every button state

  lv_style_init(&st_btn_sub);
  lv_style_set_text_font(&st_btn_sub, theme_font_name);
  lv_style_set_text_opa(&st_btn_sub, LV_OPA_70);

  lv_style_init(&st_list);
  lv_style_set_bg_color(&st_list, lv_color_black());
  lv_style_set_bg_opa(&st_list, LV_OPA_COVER);
  lv_style_set_border_width(&st_list, 0);
  lv_style_set_radius(&st_list, 0);
  lv_style_set_pad_all(&st_list, 0);
  lv_style_set_pad_row(&st_list, 6);

  lv_style_init(&st_list_row);
  lv_style_set_text_font(&st_list_row, theme_font_value);
  lv_style_set_text_color(&st_list_row, theme_text());
  lv_style_set_pad_ver(&st_list_row, 10);
  lv_style_set_pad_hor(&st_list_row, 16);
  lv_style_set_radius(&st_list_row, 8);
  lv_style_set_bg_opa(&st_list_row, LV_OPA_TRANSP);

  lv_style_init(&st_list_row_sel);
  lv_style_set_bg_color(&st_list_row_sel, theme_surface());
  lv_style_set_bg_opa(&st_list_row_sel, LV_OPA_COVER);
  lv_style_set_text_color(&st_list_row_sel, theme_amber());

  lv_style_init(&st_group_title);
  lv_style_set_text_font(&st_group_title, theme_font_small);
  lv_style_set_text_color(&st_group_title, theme_amber());
  lv_style_set_text_letter_space(&st_group_title, 2);

  lv_style_init(&st_editor);
  lv_style_set_bg_color(&st_editor, lv_color_black());
  lv_style_set_bg_opa(&st_editor, LV_OPA_80);
  lv_style_set_border_width(&st_editor, 0);
  lv_style_set_radius(&st_editor, 0);
  lv_style_set_pad_all(&st_editor, 0);

  lv_style_init(&st_editor_value);
  lv_style_set_text_font(&st_editor_value, theme_font_big);
  lv_style_set_text_color(&st_editor_value, theme_text());
  lv_style_set_text_align(&st_editor_value, LV_TEXT_ALIGN_CENTER);
  lv_style_set_bg_opa(&st_editor_value, LV_OPA_TRANSP);
  lv_style_set_border_width(&st_editor_value, 0);
  lv_style_set_pad_all(&st_editor_value, 4);

  lv_style_init(&st_editor_title);
  lv_style_set_text_font(&st_editor_title, theme_font_big);
  lv_style_set_text_color(&st_editor_title, theme_muted());
  lv_style_set_text_align(&st_editor_title, LV_TEXT_ALIGN_CENTER);

  lv_style_init(&st_field_label);
  lv_style_set_text_font(&st_field_label, theme_font_name);
  lv_style_set_text_color(&st_field_label, theme_muted());

  lv_style_init(&st_field);
  lv_style_set_text_font(&st_field, theme_font_name);
  lv_style_set_text_color(&st_field, theme_text());
  lv_style_set_bg_color(&st_field, theme_surface());
  lv_style_set_bg_opa(&st_field, LV_OPA_COVER);
  lv_style_set_border_color(&st_field, theme_steel());
  lv_style_set_border_width(&st_field, 1);
  lv_style_set_radius(&st_field, 6);
  lv_style_set_pad_ver(&st_field, 6);
  lv_style_set_pad_hor(&st_field, 10);

  lv_style_init(&st_view);
  lv_style_set_bg_opa(&st_view, LV_OPA_TRANSP);
  lv_style_set_border_width(&st_view, 0);
  lv_style_set_pad_all(&st_view, 12);
  lv_style_set_pad_row(&st_view, 10);
  lv_style_set_pad_column(&st_view, 12);

  lv_style_init(&st_btn_value);
  lv_style_set_text_font(&st_btn_value, theme_font_big);

  lv_style_init(&st_tile);
  lv_style_set_bg_color(&st_tile, lv_color_black());  // readouts on pure black
  lv_style_set_bg_opa(&st_tile, LV_OPA_COVER);
  lv_style_set_border_width(&st_tile, 0);
  lv_style_set_radius(&st_tile, 6);
  lv_style_set_shadow_width(&st_tile, 0);
  lv_style_set_pad_all(&st_tile, 0);
  lv_style_set_clip_corner(&st_tile, true);

  lv_style_init(&st_tile_label);
  lv_style_set_text_color(&st_tile_label, theme_muted());
  lv_style_set_text_font(&st_tile_label, theme_font_small);
  lv_style_set_text_letter_space(&st_tile_label, 2);

  lv_style_init(&st_tile_value);
  lv_style_set_text_color(&st_tile_value, theme_text());
  lv_style_set_text_font(&st_tile_value, theme_font_big);

  lv_style_init(&st_tile_unit);
  lv_style_set_text_color(&st_tile_unit, theme_muted());
  lv_style_set_text_font(&st_tile_unit, theme_font_name);

  lv_style_init(&st_chart);
  lv_style_set_bg_opa(&st_chart, LV_OPA_TRANSP);
  lv_style_set_border_width(&st_chart, 0);
  lv_style_set_pad_all(&st_chart, 0);
  lv_style_set_radius(&st_chart, 0);

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

void theme_apply_header_notes(lv_obj_t *label) { lv_obj_add_style(label, &st_header_notes, 0); }
void theme_apply_menu(lv_obj_t *dd) {
  lv_obj_add_style(dd, &st_menu, 0);
  lv_obj_add_style(dd, &st_btn_dis, LV_STATE_DISABLED);
}
void theme_apply_tile(lv_obj_t *tile) { lv_obj_add_style(tile, &st_tile, 0); }
void theme_apply_tile_label(lv_obj_t *label) { lv_obj_add_style(label, &st_tile_label, 0); }
void theme_apply_tile_value(lv_obj_t *label) { lv_obj_add_style(label, &st_tile_value, 0); }
void theme_apply_tile_unit(lv_obj_t *label) { lv_obj_add_style(label, &st_tile_unit, 0); }
void theme_apply_btn_name(lv_obj_t *label) { lv_obj_add_style(label, &st_btn_name, 0); }
void theme_apply_btn_value(lv_obj_t *label) { lv_obj_add_style(label, &st_btn_value, 0); }
void theme_apply_btn_sub(lv_obj_t *label) { lv_obj_add_style(label, &st_btn_sub, 0); }
void theme_apply_list(lv_obj_t *list) { lv_obj_add_style(list, &st_list, 0); }
void theme_apply_list_row(lv_obj_t *label) {
  lv_obj_add_style(label, &st_list_row, 0);
  lv_obj_add_style(label, &st_list_row_sel, LV_STATE_CHECKED);
}
void theme_set_selected(lv_obj_t *label, bool selected) {
  if (selected) lv_obj_add_state(label, LV_STATE_CHECKED);
  else lv_obj_clear_state(label, LV_STATE_CHECKED);
}
void theme_apply_group_title(lv_obj_t *label) { lv_obj_add_style(label, &st_group_title, 0); }
void theme_apply_field_label(lv_obj_t *label) { lv_obj_add_style(label, &st_field_label, 0); }
void theme_apply_field(lv_obj_t *ta) {
  lv_obj_add_style(ta, &st_field, 0);
  lv_obj_set_style_border_color(ta, theme_amber(), LV_STATE_FOCUSED);
}
void theme_apply_view(lv_obj_t *view) { lv_obj_add_style(view, &st_view, 0); }

void theme_apply_graph(lv_obj_t *chart) {
  lv_obj_add_style(chart, &st_chart, 0);
  lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
  lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
  lv_obj_set_style_line_opa(chart, LV_OPA_90, LV_PART_ITEMS);
  lv_obj_set_style_line_color(chart, theme_steel(), LV_PART_MAIN);
  lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
  lv_obj_set_style_line_opa(chart, LV_OPA_50, LV_PART_MAIN);
  lv_chart_set_div_line_count(chart, 4, 0);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_obj_clear_flag(chart, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

void theme_apply_editor(lv_obj_t *overlay) { lv_obj_add_style(overlay, &st_editor, 0); }
void theme_apply_editor_title(lv_obj_t *label) { lv_obj_add_style(label, &st_editor_title, 0); }
void theme_apply_editor_value(lv_obj_t *ta) {
  lv_obj_add_style(ta, &st_editor_value, 0);
  lv_obj_set_style_text_color(ta, theme_amber(), LV_PART_CURSOR);
}

void theme_apply_legend(lv_obj_t *label, lv_color_t color) {
  lv_obj_set_style_text_font(label, theme_font_name, 0);
  lv_obj_set_style_text_color(label, color, 0);
}

void theme_apply_chart(lv_obj_t *chart) {
  lv_obj_add_style(chart, &st_chart, 0);
  lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);        // no points
  lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
  lv_obj_set_style_line_opa(chart, LV_OPA_60, LV_PART_ITEMS);
  lv_chart_set_div_line_count(chart, 0, 0);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
  lv_obj_clear_flag(chart, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

lv_chart_series_t *theme_chart_series(lv_obj_t *chart) {
  return lv_chart_add_series(chart, steel_hi(), LV_CHART_AXIS_PRIMARY_Y);
}

void theme_set_hot(lv_obj_t *value, lv_obj_t *chart, lv_chart_series_t *ser, bool hot) {
  lv_obj_set_style_text_color(value, hot ? theme_amber() : theme_text(), 0);
  if (chart && ser) lv_chart_set_series_color(chart, ser, hot ? theme_amber() : steel_hi());
}

void theme_apply_btn(lv_obj_t *btn, bool steam) {
  lv_obj_add_style(btn, &st_btn, 0);
  lv_obj_add_style(btn, steam ? &st_btn_on_steam : &st_btn_on, LV_STATE_CHECKED);
  lv_obj_add_style(btn, &st_btn_dis, LV_STATE_DISABLED);
}
