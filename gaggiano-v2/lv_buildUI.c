/**
 * @file lv_buildUI.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_buildUI.h"
#include "my_logging.h"

#if LV_MEM_CUSTOM == 0 && LV_MEM_SIZE < (38ul * 1024ul)
#error Insufficient memory for lv_demo_widgets. Please set LV_MEM_SIZE to at least 38KB (38ul * 1024ul).  48KB is recommended.
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/

static void basic_create(lv_obj_t* parent);
static void settings_create(lv_obj_t* parent);
static void profile_create(lv_obj_t* parent);
static void analytics_create(lv_obj_t* parent);
static void shop_create(lv_obj_t* parent);
static void color_changer_create(lv_obj_t* parent);

/**********************
 *  STATIC VARIABLES
 **********************/

static lv_obj_t* tempSet2Label;
static lv_obj_t* tempRead2Label;
static lv_obj_t* pressureSet2Label;
static lv_obj_t* pressureRead2Label;
static lv_obj_t* brew_temp_tf;
static lv_obj_t* brew_pressure_tf;
static lv_obj_t* steam_temp_tf;
static lv_obj_t* setBtn;

static const lv_font_t* font_large;
static const lv_font_t* font_normal;
static lv_obj_t* tv;
static lv_obj_t* calendar;
static lv_style_t style_text_muted;
static lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;

/**********************
 *      MACROS
 **********************/
/**********************
 *      EVENT HANDLING
 **********************/

static void setting_field_changed(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* ta = lv_event_get_target(e);
  lv_obj_t* kb = lv_event_get_user_data(e);
  if (code == LV_EVENT_FOCUSED) {
    if (lv_indev_get_type(lv_indev_get_act()) != LV_INDEV_TYPE_KEYPAD) {
      lv_keyboard_set_textarea(kb, ta);
      lv_obj_set_style_max_height(kb, LV_HOR_RES * 2 / 3, 0);
      lv_obj_update_layout(tv); /*Be sure the sizes are recalculated*/
      lv_obj_set_height(tv, LV_VER_RES - lv_obj_get_height(kb));
      lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
      lv_obj_scroll_to_view_recursive(ta, LV_ANIM_OFF);
    }
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_keyboard_set_textarea(kb, NULL);
    lv_obj_set_height(tv, LV_VER_RES);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_indev_reset(NULL, ta);

  } else if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    lv_obj_set_height(tv, LV_VER_RES);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(ta, LV_STATE_FOCUSED);
    lv_indev_reset(NULL, ta); /*To forget the last clicked object to make it focusable again*/
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    LV_LOG_WARN("Setting changed");
    lv_obj_clear_state(setBtn, LV_STATE_DISABLED);
  }
}


static void setButtonClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("Button Clicked");
    double newBoilerSetPoint = strtod(lv_textarea_get_text(brew_temp_tf),NULL);
    double newPressureSetPoint = strtod(lv_textarea_get_text(brew_pressure_tf),NULL);
    double newSteamSetPoint = strtod(lv_textarea_get_text(steam_temp_tf),NULL);
    boilerSetPoint = newBoilerSetPoint;
    pressureSetPoint = newPressureSetPoint;
    steamSetPoint = newSteamSetPoint;
    hasChanged = true;
    writeConfigFile();
    lv_obj_add_state(setBtn, LV_STATE_DISABLED);
  }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void my_log_cb(const char* buf) {
  my_log(buf);
}
void updateUI() {
  lv_label_set_text_fmt(tempRead2Label, "%.2f", tempRead);
  lv_label_set_text_fmt(pressureRead2Label, "%.2f", pressureRead);

  if (hasChanged) {

    lv_label_set_text_fmt(tempSet2Label, "%.2f", boilerSetPoint);
    lv_label_set_text_fmt(pressureSet2Label, "%.2f", pressureSetPoint);

    char t[10];
    sprintf(t, "%.2f", boilerSetPoint);
    lv_textarea_set_text(brew_temp_tf, t);

    sprintf(t, "%.2f", pressureSetPoint);
    lv_textarea_set_text(brew_pressure_tf, t);

    sprintf(t, "%.2f", steamSetPoint);
    lv_textarea_set_text(steam_temp_tf, t);

    hasChanged = false;
  }
}

void instantiateUI() {
  lv_log_register_print_cb(my_log_cb);

  LV_LOG_ERROR("logging works!!");

  font_large = LV_FONT_DEFAULT;
  font_normal = LV_FONT_DEFAULT;

  lv_coord_t tab_h;

  tab_h = 70;
#if LV_FONT_MONTSERRAT_24
  font_large = &lv_font_montserrat_24;
#else
  LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_16
  font_normal = &lv_font_montserrat_16;
#else
  LV_LOG_WARN("LV_FONT_MONTSERRAT_16 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif


#if LV_USE_THEME_DEFAULT
  lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
                        font_normal);
#endif

  lv_style_init(&style_text_muted);
  lv_style_set_text_opa(&style_text_muted, LV_OPA_50);

  lv_style_init(&style_title);
  lv_style_set_text_font(&style_title, font_large);

  lv_style_init(&style_icon);
  lv_style_set_text_color(&style_icon, lv_theme_get_color_primary(NULL));
  lv_style_set_text_font(&style_icon, font_large);

  lv_style_init(&style_bullet);
  lv_style_set_border_width(&style_bullet, 0);
  lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

  tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, tab_h);

  lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);


  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tv);
  lv_obj_set_style_pad_left(tab_btns, LV_HOR_RES / 2, 0);
  lv_obj_t* logo = lv_img_create(tab_btns);
  LV_IMG_DECLARE(img_lvgl_logo);
  lv_img_set_src(logo, &img_lvgl_logo);
  lv_obj_align(logo, LV_ALIGN_LEFT_MID, -LV_HOR_RES / 2 + 25, 0);

  lv_obj_t* label = lv_label_create(tab_btns);
  lv_obj_add_style(label, &style_title, 0);
  lv_label_set_text(label, "Gaggiano Basic V2");
  lv_obj_align_to(label, logo, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);

  label = lv_label_create(tab_btns);
  lv_label_set_text(label, "In Development");
  lv_obj_add_style(label, &style_text_muted, 0);
  lv_obj_align_to(label, logo, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);

  lv_obj_t* t1 = lv_tabview_add_tab(tv, "Gaggia Basic");
  lv_obj_t* t2 = lv_tabview_add_tab(tv, "Settings");
  basic_create(t1);
  settings_create(t2);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void basic_create(lv_obj_t* parent) {

  lv_obj_t* boilerBtn = lv_btn_create(parent);
  //lv_obj_add_event_cb(boilerBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(boilerBtn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(boilerBtn, 100);
  lv_obj_set_width(boilerBtn, 100);

  lv_obj_t* boilerBtnLabel = lv_label_create(boilerBtn);
  lv_label_set_text(boilerBtnLabel, "Boiler");
  lv_obj_center(boilerBtnLabel);

  lv_obj_t* brewBtn = lv_btn_create(parent);
  //lv_obj_add_event_cb(boilerBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(brewBtn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(brewBtn, 100);
  lv_obj_set_width(brewBtn, 100);

  lv_obj_t* brewBtnLabel = lv_label_create(brewBtn);
  lv_label_set_text(brewBtnLabel, "Brew");
  lv_obj_center(brewBtnLabel);

  lv_obj_t* steamBtn = lv_btn_create(parent);
  //lv_obj_add_event_cb(boilerBtn, event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_flag(steamBtn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(steamBtn, 100);
  lv_obj_set_width(steamBtn, 100);

  lv_obj_t* steamBtnLabel = lv_label_create(steamBtn);
  lv_label_set_text(steamBtnLabel, "Steam");
  lv_obj_center(steamBtnLabel);

  lv_obj_t* panel1 = lv_obj_create(parent);
  lv_obj_set_height(panel1, LV_SIZE_CONTENT);

  lv_obj_t* tempSet1Label = lv_label_create(panel1);
  lv_label_set_text(tempSet1Label, "Temp Set:");

  tempSet2Label = lv_label_create(panel1);
  lv_label_set_text_fmt(tempSet2Label, "%.2f", boilerSetPoint);

  lv_obj_t* tempRead1Label = lv_label_create(panel1);
  lv_label_set_text(tempRead1Label, "Temp Read:");

  tempRead2Label = lv_label_create(panel1);
  lv_label_set_text_fmt(tempRead2Label, "%.2f", tempRead);

  lv_obj_t* pressureSet1Label = lv_label_create(panel1);
  lv_label_set_text(pressureSet1Label, "Pressure Set:");

  pressureSet2Label = lv_label_create(panel1);
  lv_label_set_text_fmt(pressureSet2Label, "%.2f", pressureSetPoint);

  lv_obj_t* pressureRead1Label = lv_label_create(panel1);
  lv_label_set_text(pressureRead1Label, "Pressure Read:");

  pressureRead2Label = lv_label_create(panel1);
  lv_label_set_text_fmt(pressureRead2Label, "%.2f", pressureRead);

  static lv_coord_t grid_panel1_col_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 20, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_panel1_row_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(panel1, grid_panel1_col_dsc, grid_panel1_row_dsc);

  lv_obj_set_grid_cell(tempSet1Label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(tempSet2Label, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(tempRead1Label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(tempRead2Label, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(pressureSet1Label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(pressureSet2Label, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(pressureRead1Label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(pressureRead2Label, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 1, 1);

  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

  lv_obj_set_grid_cell(boilerBtn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(brewBtn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(steamBtn, LV_GRID_ALIGN_STRETCH, 4, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 5, LV_GRID_ALIGN_STRETCH, 1, 1);
}

static void settings_create(lv_obj_t* parent) {
  lv_obj_t* panel1 = lv_obj_create(parent);

  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);
  lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  lv_obj_t* brew_temp_label = lv_label_create(panel1);
  lv_label_set_text(brew_temp_label, "Brew Temperature:");

  lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

  brew_temp_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(brew_temp_tf, true);
  char t[6];
  sprintf(t, "%.2f", boilerSetPoint);
  lv_textarea_set_text(brew_temp_tf, t);
  lv_obj_add_event_cb(brew_temp_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* brew_pressure_label = lv_label_create(panel1);
  lv_label_set_text(brew_pressure_label, "Brew Pressure:");

  brew_pressure_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(brew_pressure_tf, true);
  sprintf(t, "%.2f", pressureSetPoint);
  lv_textarea_set_text(brew_pressure_tf, t);
  lv_obj_add_event_cb(brew_pressure_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* steam_temp_label = lv_label_create(panel1);
  lv_label_set_text(steam_temp_label, "Steam Temperature:");

  steam_temp_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(steam_temp_tf, true);
  sprintf(t, "%.2f", steamSetPoint);
  lv_textarea_set_text(steam_temp_tf, t);
  lv_obj_add_event_cb(steam_temp_tf, setting_field_changed, LV_EVENT_ALL, kb);

  setBtn = lv_btn_create(panel1);
  lv_obj_t* setBtn_label = lv_label_create(setBtn);
  lv_label_set_text(setBtn_label, "Set");
  lv_obj_add_state(setBtn, LV_STATE_DISABLED);
  lv_obj_add_event_cb(setBtn, setButtonClicked, LV_EVENT_ALL, kb);

  lv_obj_t* cancelBtn = lv_btn_create(panel1);
  lv_obj_t* cancelBtn_Label = lv_label_create(cancelBtn);
  lv_label_set_text(cancelBtn_Label, "Cancel");

  static lv_coord_t grid_panel1_col_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_panel1_row_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(panel1, grid_panel1_col_dsc, grid_panel1_row_dsc);

  lv_obj_set_grid_cell(brew_temp_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(brew_temp_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(brew_pressure_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(brew_pressure_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(steam_temp_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(steam_temp_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(setBtn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 6, 1);
  lv_obj_set_grid_cell(cancelBtn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 6, 1);
}



// #endif
