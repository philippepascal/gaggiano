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

static int (*writeConfigFile)();
static char* (*listProfiles)();
static char* (*getCurrentProfile)();
static int (*writeCurrentProfile)(const char* profileName);
static int (*setupAndReadConfigFile)();

static void basic_create(lv_obj_t* parent);
static void settings_create(lv_obj_t* parent);
static void advancedSettings_create(lv_obj_t* parent);
static void profile_create(lv_obj_t* parent);
static void analytics_create(lv_obj_t* parent);
static void shop_create(lv_obj_t* parent);
static void color_changer_create(lv_obj_t* parent);
static void updateProfileTab();

/**********************
 *  STATIC VARIABLES
 **********************/
static GaggiaStateT* state;
static AdvancedSettingsT* advancedSettings;

static lv_obj_t* tempSet2Label;
static lv_obj_t* tempRead2Label;
static lv_obj_t* pressureSet2Label;
static lv_obj_t* pressureRead2Label;
static lv_obj_t* solenoid2Label;
static lv_obj_t* lastBrewTimeLabel;

static lv_obj_t* fileList;
static lv_obj_t* fileName_tf;

static lv_obj_t* brew_temp_tf;
static lv_obj_t* brew_pressure_tf;
static lv_obj_t* steam_temp_tf;
static lv_obj_t* steam_max_pressure_tf;
static lv_obj_t* steam_pump_output_perc_tf;
static lv_obj_t* blooming_pressure_tf;
static lv_obj_t* blooming_fill_time_tf;
static lv_obj_t* blooming_wait_time_tf;
static lv_obj_t* brew_timer_tf;
static lv_obj_t* setBtn;
static lv_obj_t* cleanBtn;
static lv_obj_t* clearLogsBtn;

static lv_obj_t* boiler_bb_range_tf;
static lv_obj_t* boiler_PID_cycle_tf;
static lv_obj_t* boiler_PID_KP_tf;
static lv_obj_t* boiler_PID_KI_tf;
static lv_obj_t* boiler_PID_KD_tf;
static lv_obj_t* pump_max_step_up_tf;
static lv_obj_t* pump_KP_tf;
static lv_obj_t* pump_KI_tf;
static lv_obj_t* pump_KD_tf;
static lv_obj_t* unused1_tf;
static lv_obj_t* advancedSetBtn;

static lv_obj_t* boilerBtn;
static lv_obj_t* brewBtn;
static lv_obj_t* steamBtn;

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
    lv_obj_clear_state(advancedSetBtn, LV_STATE_DISABLED);
  }
}

static void tab_changed(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    if (lv_tabview_get_tab_act(tv) == 1) {
      //profile is selected.
      LV_LOG_WARN(" ---------- Profile Selected ----------");
      updateProfileTab();
    }
  }
}
static void setButtonClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("Set Button Clicked");
    double newBoilerSetPoint = strtod(lv_textarea_get_text(brew_temp_tf), NULL);
    double newPressureSetPoint = strtod(lv_textarea_get_text(brew_pressure_tf), NULL);
    double newSteamSetPoint = strtod(lv_textarea_get_text(steam_temp_tf), NULL);
    double newSteamMaxPress = strtod(lv_textarea_get_text(steam_max_pressure_tf), NULL);
    double newSteamPumpOutput = strtod(lv_textarea_get_text(steam_pump_output_perc_tf), NULL);
    double newblooming_pressure = strtod(lv_textarea_get_text(blooming_pressure_tf), NULL);
    double newblooming_fill_time = strtod(lv_textarea_get_text(blooming_fill_time_tf), NULL);
    double newblooming_wait_time = strtod(lv_textarea_get_text(blooming_wait_time_tf), NULL);
    double newbrew_timer = strtod(lv_textarea_get_text(brew_timer_tf), NULL);
    state->boilerSetPoint = newBoilerSetPoint;
    state->pressureSetPoint = newPressureSetPoint;
    state->steamSetPoint = newSteamSetPoint;
    state->steam_max_pressure = newSteamMaxPress;
    state->steam_pump_output_percent = newSteamPumpOutput;
    state->blooming_pressure = newblooming_pressure;
    state->blooming_fill_time = newblooming_fill_time;
    state->blooming_wait_time = newblooming_wait_time;
    state->brew_timer = newbrew_timer;
    state->hasConfigChanged = true;
    writeConfigFile();
    lv_obj_add_state(setBtn, LV_STATE_DISABLED);
  }
}
static void cancelButtonClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("Cancel Button Clicked");
    state->hasConfigChanged = true;
    lv_obj_add_state(setBtn, LV_STATE_DISABLED);
  }
}


static void advancedSetButtonClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("Set Button Clicked");

    advancedSettings->boiler_bb_range = strtod(lv_textarea_get_text(boiler_bb_range_tf), NULL);
    advancedSettings->boiler_PID_cycle = strtod(lv_textarea_get_text(boiler_PID_cycle_tf), NULL);
    advancedSettings->boiler_PID_KP = strtod(lv_textarea_get_text(boiler_PID_KP_tf), NULL);
    advancedSettings->boiler_PID_KI = strtod(lv_textarea_get_text(boiler_PID_KI_tf), NULL);
    advancedSettings->boiler_PID_KD = strtod(lv_textarea_get_text(boiler_PID_KD_tf), NULL);
    advancedSettings->pump_max_step_up = strtod(lv_textarea_get_text(pump_max_step_up_tf), NULL);
    advancedSettings->pump_KP = strtod(lv_textarea_get_text(pump_KP_tf), NULL);
    advancedSettings->pump_KI = strtod(lv_textarea_get_text(pump_KI_tf), NULL);
    advancedSettings->pump_KD = strtod(lv_textarea_get_text(pump_KD_tf), NULL);
    advancedSettings->unused1 = strtod(lv_textarea_get_text(unused1_tf), NULL);

    advancedSettings->userChanged = true;
    advancedSettings->sendToController = true;

    writeConfigFile();
    lv_obj_add_state(advancedSetBtn, LV_STATE_DISABLED);
  }
}
static void advancedCancelButtonClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("Cancel Button Clicked");
    advancedSettings->userChanged = true;
    lv_obj_add_state(advancedSetBtn, LV_STATE_DISABLED);
  }
}

static void boilerButtonClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    LV_LOG_USER("boiler Toggled");
    if (lv_obj_get_state(boilerBtn) & LV_STATE_CHECKED) {
      //set arduino to boilerSetPoint
      LV_LOG_USER("starting boiler");
      state->isBoilerOn = true;
      state->hasCommandChanged = true;
    } else {
      //set arduino to boilerSetPoint 0
      LV_LOG_USER("stopping boiler (and steam)");
      state->isBoilerOn = false;
      lv_obj_clear_state(steamBtn, LV_STATE_CHECKED);
      lv_imgbtn_set_state(steamBtn, LV_IMGBTN_STATE_RELEASED);
      state->isSteaming = false;
      state->hasCommandChanged = true;
    }
  }
}
static void brewButtonClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    LV_LOG_USER("brew Clicked");
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    LV_LOG_USER("brew Toggled");
    if (lv_obj_get_state(brewBtn) & LV_STATE_CHECKED) {
      //set arduino to brew (valve and pump) to pressureSetPoint
      LV_LOG_USER("starting brew");
      state->isBrewing = true;
      state->hasCommandChanged = true;
    } else {
      //set arduino to brew (valve and pump) to pressureSetPoint 0
      LV_LOG_USER("stopping brew");
      state->isBrewing = false;
      state->hasCommandChanged = true;
    }
  }
}
static void steamButtonClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    LV_LOG_USER("steam Clicked");
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    LV_LOG_USER("steam Toggled");
    if (lv_obj_get_state(steamBtn) & LV_STATE_CHECKED) {
      //set arduino to boilerSetPoint at steamSetPoint
      LV_LOG_USER("starting steam (and boiler)");
      state->isBoilerOn = true;
      lv_obj_add_state(boilerBtn, LV_STATE_CHECKED);
      lv_imgbtn_set_state(boilerBtn, LV_IMGBTN_STATE_CHECKED_RELEASED);
      state->isSteaming = true;
      state->hasCommandChanged = true;
    } else {
      //set arduino to boilerSetPoint to 0
      LV_LOG_USER("stopping steam ");
      state->isSteaming = false;
      state->hasCommandChanged = true;
    }
  }
}

static void cleanBtnClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("Clean Button Clicked");
    if (state->isBrewing) {
      LV_LOG_USER("no cleaning while brewing");
    } else {
      state->isCleaning = true;
      state->hasCommandChanged = true;
    }
  }
}
static void clearLogsBtnClicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("Clear Logs Button Clicked");
    state->cleanLogs = true;
  }
}
static void profile_selected(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
  LV_LOG_WARN("profile label Clicked");
    const char* clickedProfileName = lv_label_get_text(lv_event_get_target(e));
    if (strlen(clickedProfileName) > 0) {
      lv_textarea_set_text(fileName_tf, clickedProfileName);
      //lv_textarea_get_text(fileName_tf);
      writeCurrentProfile(clickedProfileName);
      setupAndReadConfigFile();
    }
  }
}
/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void my_log_cb(const char* buf) {
  my_log(buf);
}

void updateUI() {
  LV_LOG_TRACE("updating real time fields");

  if (state->isSteaming)
    lv_label_set_text_fmt(tempSet2Label, "%.2f", state->steamSetPoint);
  else
    lv_label_set_text_fmt(tempSet2Label, "%.2f", state->boilerSetPoint);
  lv_label_set_text_fmt(pressureSet2Label, "%.2f", state->pressureSetPoint);

  lv_label_set_text_fmt(tempRead2Label, "%.2f", state->tempRead);
  lv_label_set_text_fmt(pressureRead2Label, "%.2f", state->pressureRead);

  if (state->isSolenoidOn) {
    lv_label_set_text_fmt(solenoid2Label, "ON");
  } else {
    lv_label_set_text_fmt(solenoid2Label, "OFF");
  }

  lv_label_set_text_fmt(lastBrewTimeLabel, "%.2f", state->lastBrewTime);

  // update buttons
  if (lv_obj_get_state(brewBtn) & LV_STATE_CHECKED) {
    if (!state->isBrewing) {
      lv_obj_clear_state(brewBtn, LV_STATE_CHECKED);
      lv_imgbtn_set_state(brewBtn, LV_IMGBTN_STATE_RELEASED);
    }
  } else {
    if (state->isBrewing) {
      lv_obj_add_state(brewBtn, LV_STATE_CHECKED);
      lv_imgbtn_set_state(brewBtn, LV_IMGBTN_STATE_CHECKED_RELEASED);
    }
  }
  if (lv_obj_get_state(steamBtn) & LV_STATE_CHECKED) {
    if (!state->isSteaming) {
      lv_obj_clear_state(steamBtn, LV_STATE_CHECKED);
      lv_imgbtn_set_state(steamBtn, LV_IMGBTN_STATE_RELEASED);
    }
  } else {
    if (state->isSteaming) {
      lv_obj_add_state(steamBtn, LV_STATE_CHECKED);
      lv_imgbtn_set_state(steamBtn, LV_IMGBTN_STATE_CHECKED_RELEASED);
    }
  }
  if (lv_obj_get_state(boilerBtn) & LV_STATE_CHECKED) {
    if (!state->isBoilerOn) {
      lv_obj_clear_state(boilerBtn, LV_STATE_CHECKED);
      lv_imgbtn_set_state(boilerBtn, LV_IMGBTN_STATE_RELEASED);
    }
  } else {
    if (state->isBoilerOn) {
      lv_obj_add_state(boilerBtn, LV_STATE_CHECKED);
      lv_imgbtn_set_state(boilerBtn, LV_IMGBTN_STATE_CHECKED_RELEASED);
    }
  }

  if (state->hasConfigChanged) {
    LV_LOG_WARN("updating config fields");

    char t[100];
    sprintf(t, "%.2f", state->boilerSetPoint);
    lv_textarea_set_text(brew_temp_tf, t);

    sprintf(t, "%.2f", state->pressureSetPoint);
    lv_textarea_set_text(brew_pressure_tf, t);

    sprintf(t, "%.2f", state->steamSetPoint);
    lv_textarea_set_text(steam_temp_tf, t);

    sprintf(t, "%.2f", state->steam_max_pressure);
    lv_textarea_set_text(steam_max_pressure_tf, t);

    sprintf(t, "%.2f", state->steam_pump_output_percent);
    lv_textarea_set_text(steam_pump_output_perc_tf, t);

    sprintf(t, "%.2f", state->blooming_pressure);
    lv_textarea_set_text(blooming_pressure_tf, t);

    sprintf(t, "%.2f", state->blooming_fill_time);
    lv_textarea_set_text(blooming_fill_time_tf, t);

    sprintf(t, "%.2f", state->blooming_wait_time);
    lv_textarea_set_text(blooming_wait_time_tf, t);

    sprintf(t, "%.2f", state->brew_timer);
    lv_textarea_set_text(brew_timer_tf, t);

    //yuk, but setting text area emits a change event...
    lv_obj_add_state(setBtn, LV_STATE_DISABLED);

    state->hasConfigChanged = false;
  }
  if (advancedSettings->userChanged) {
    LV_LOG_WARN("updating advanced Settings fields");

    char t[100];
    sprintf(t, "%.2f", advancedSettings->boiler_bb_range);
    lv_textarea_set_text(boiler_bb_range_tf, t);
    sprintf(t, "%.2f", advancedSettings->boiler_PID_cycle);
    lv_textarea_set_text(boiler_PID_cycle_tf, t);
    sprintf(t, "%.2f", advancedSettings->boiler_PID_KP);
    lv_textarea_set_text(boiler_PID_KP_tf, t);
    sprintf(t, "%.2f", advancedSettings->boiler_PID_KI);
    lv_textarea_set_text(boiler_PID_KI_tf, t);
    sprintf(t, "%.2f", advancedSettings->boiler_PID_KD);
    lv_textarea_set_text(boiler_PID_KD_tf, t);
    sprintf(t, "%.2f", advancedSettings->pump_max_step_up);
    lv_textarea_set_text(pump_max_step_up_tf, t);
    sprintf(t, "%.2f", advancedSettings->pump_KP);
    lv_textarea_set_text(pump_KP_tf, t);
    sprintf(t, "%.2f", advancedSettings->pump_KI);
    lv_textarea_set_text(pump_KI_tf, t);
    sprintf(t, "%.2f", advancedSettings->pump_KD);
    lv_textarea_set_text(pump_KD_tf, t);
    sprintf(t, "%.2f", advancedSettings->unused1);
    lv_textarea_set_text(unused1_tf, t);

    //yuk, but setting text area emits a change event...
    lv_obj_add_state(advancedSetBtn, LV_STATE_DISABLED);

    advancedSettings->userChanged = false;
  }
}

void instantiateUI(GaggiaStateT* s,
                   AdvancedSettingsT* as,
                   int (*f)(),
                   char* (*lp)(),
                   char* (*gcp)(),
                   int (*wcp)(const char* profileName),
                   int (*sarcf)()) {
  state = s;
  advancedSettings = as;
  writeConfigFile = f;
  listProfiles = lp;
  getCurrentProfile = gcp;
  writeCurrentProfile = wcp;
  setupAndReadConfigFile = sarcf;

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
  lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_GREY), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
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
  lv_obj_add_event_cb(tv, tab_changed, LV_EVENT_ALL, NULL);

  lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);


  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tv);
  lv_obj_set_style_pad_left(tab_btns, LV_HOR_RES / 2, 0);
  lv_obj_t* logo = lv_img_create(tab_btns);
  LV_IMG_DECLARE(logoMainScreen);
  lv_img_set_src(logo, &logoMainScreen);
  lv_obj_align(logo, LV_ALIGN_LEFT_MID, -LV_HOR_RES / 2 + 25, 0);


  lv_obj_t* t1 = lv_tabview_add_tab(tv, "Brew");
  lv_obj_t* t4 = lv_tabview_add_tab(tv, "Profile");
  lv_obj_t* t2 = lv_tabview_add_tab(tv, "Settings");
  lv_obj_t* t3 = lv_tabview_add_tab(tv, "Advanced");


  basic_create(t1);
  profile_create(t4);
  settings_create(t2);
  advancedSettings_create(t3);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

int myIndexOf(const char* str, const char ch, int fromIndex) {
  const char* result = strchr(str + fromIndex, ch);
  if (result == NULL) {
    return -1;  // Substring not found
  } else {
    return result - str;  // Calculate the index
  }
}

char* mySubString(const char* str, int start, int end) {
  char* sub = (char*)malloc(sizeof(char) * (end - start));
  if (sub == NULL) {
    return NULL;
  }

  strncpy(sub, str + start, (end - start));
  sub[(end - start)] = '\0';

  return sub;
}

void updateProfileTab() {
  const char* profileNames = listProfiles();
  LV_LOG_WARN(profileNames);
  int start = 0;
  int end = myIndexOf(profileNames, ';', start);
  int index = 0;
  while (end > 0) {
    const char* profileName = mySubString(profileNames, start, end);
    LV_LOG_WARN(profileName);
    lv_obj_t* child = lv_obj_get_child(fileList, index);
    lv_label_set_text(child, profileName);
    start = end + 1;
    end = myIndexOf(profileNames, ';', start);
    index++;
  }
  for (; index < 10; index++) {
    lv_obj_t* child = lv_obj_get_child(fileList, index);
    lv_label_set_text(child, "N/A");
  }

  const char* fn = getCurrentProfile();
  lv_textarea_set_text(fileName_tf, fn);
}

static void profile_create(lv_obj_t* parent) {

  fileList = lv_list_create(parent);
  // lv_obj_set_size(fileList, lv_pct(60), lv_pct(100));
  lv_obj_set_style_pad_row(fileList, 5, 0);

  lv_obj_t* lbl;
  int i;
  for (i = 0; i < 10; i++) {
    lbl = lv_label_create(fileList);
    lv_obj_set_width(lbl, lv_pct(100));
    lv_obj_set_height(lbl, 30);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lbl, profile_selected, LV_EVENT_ALL, NULL);
    lv_label_set_text(lbl, "");
  }

  lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

  fileName_tf = lv_textarea_create(parent);
  lv_textarea_set_one_line(fileName_tf, true);
  lv_obj_add_event_cb(fileName_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* fileName_btn = lv_btn_create(parent);
  // lv_obj_add_event_cb(boilerBtn, fileName_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* fileName_btn_lbl = lv_label_create(fileName_btn);
  lv_label_set_text(fileName_btn_lbl, "Rename");
  lv_obj_center(fileName_btn_lbl);

  lv_obj_t* duplicate_btn = lv_btn_create(parent);
  // lv_obj_add_event_cb(boilerBtn, duplicate_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* duplicate_btn_lbl = lv_label_create(duplicate_btn);
  lv_label_set_text(duplicate_btn_lbl, "Duplicate");
  lv_obj_center(duplicate_btn_lbl);

  lv_obj_t* delete_btn = lv_btn_create(parent);
  // lv_obj_add_event_cb(delete_btn, delete_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* delete_btn_lbl = lv_label_create(delete_btn);
  lv_label_set_text(delete_btn_lbl, "Delete");
  lv_obj_center(delete_btn_lbl);

  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

  lv_obj_set_grid_cell(fileList, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 8);

  lv_obj_set_grid_cell(fileName_tf, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(fileName_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(duplicate_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 4, 1);
  lv_obj_set_grid_cell(delete_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 6, 1);
}

static void basic_create(lv_obj_t* parent) {

  LV_IMG_DECLARE(boiler);
  LV_IMG_DECLARE(boilerRed);
  LV_IMG_DECLARE(brew);
  LV_IMG_DECLARE(brewRed);
  LV_IMG_DECLARE(steam);
  LV_IMG_DECLARE(steamRed);

  // boilerBtn = lv_btn_create(parent);
  boilerBtn = lv_imgbtn_create(parent);
  lv_imgbtn_set_src(boilerBtn, LV_IMGBTN_STATE_RELEASED, NULL, &boiler, NULL);
  lv_imgbtn_set_src(boilerBtn, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &boilerRed, NULL);
  lv_obj_add_flag(boilerBtn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(boilerBtn, 150);
  lv_obj_set_width(boilerBtn, 150);
  lv_obj_align(boilerBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(boilerBtn, boilerButtonClicked, LV_EVENT_ALL, NULL);

  // lv_obj_t* boilerBtnLabel = lv_label_create(boilerBtn);
  // lv_label_set_text(boilerBtnLabel, "Boiler");
  // lv_obj_center(boilerBtnLabel);

  // brewBtn = lv_btn_create(parent);
  brewBtn = lv_imgbtn_create(parent);
  lv_imgbtn_set_src(brewBtn, LV_IMGBTN_STATE_RELEASED, NULL, &brew, NULL);
  lv_imgbtn_set_src(brewBtn, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &brewRed, NULL);
  lv_obj_add_flag(brewBtn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(brewBtn, 150);
  lv_obj_set_width(brewBtn, 150);
  lv_obj_align(brewBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(brewBtn, brewButtonClicked, LV_EVENT_ALL, NULL);

  // lv_obj_t* brewBtnLabel = lv_label_create(brewBtn);
  // lv_label_set_text(brewBtnLabel, "Brew");
  // lv_obj_center(brewBtnLabel);

  // steamBtn = lv_btn_create(parent);
  steamBtn = lv_imgbtn_create(parent);
  lv_imgbtn_set_src(steamBtn, LV_IMGBTN_STATE_RELEASED, NULL, &steam, NULL);
  lv_imgbtn_set_src(steamBtn, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &steamRed, NULL);
  lv_obj_add_flag(steamBtn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(steamBtn, 150);
  lv_obj_set_width(steamBtn, 150);
  lv_obj_align(steamBtn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(steamBtn, steamButtonClicked, LV_EVENT_ALL, NULL);

  // lv_obj_t* steamBtnLabel = lv_label_create(steamBtn);
  // lv_label_set_text(steamBtnLabel, "Steam");
  // lv_obj_center(steamBtnLabel);

  lv_obj_t* panel1 = lv_obj_create(parent);
  lv_obj_set_height(panel1, LV_SIZE_CONTENT);

  lv_obj_t* tempSet1Label = lv_label_create(panel1);
  lv_label_set_text(tempSet1Label, "Temp Set:");

  tempSet2Label = lv_label_create(panel1);
  lv_label_set_text_fmt(tempSet2Label, "%.2f", state->boilerSetPoint);

  lv_obj_t* tempRead1Label = lv_label_create(panel1);
  lv_label_set_text(tempRead1Label, "Temp Read:");

  tempRead2Label = lv_label_create(panel1);
  lv_label_set_text_fmt(tempRead2Label, "%.2f", state->tempRead);

  lv_obj_t* pressureSet1Label = lv_label_create(panel1);
  lv_label_set_text(pressureSet1Label, "Pressure Set:");

  pressureSet2Label = lv_label_create(panel1);
  lv_label_set_text_fmt(pressureSet2Label, "%.2f", state->pressureSetPoint);

  lv_obj_t* pressureRead1Label = lv_label_create(panel1);
  lv_label_set_text(pressureRead1Label, "Pressure Read:");

  pressureRead2Label = lv_label_create(panel1);
  lv_label_set_text_fmt(pressureRead2Label, "%.2f", state->pressureRead);

  lv_obj_t* solenoid1Label = lv_label_create(panel1);
  lv_label_set_text(solenoid1Label, "Solenoid State:");

  solenoid2Label = lv_label_create(panel1);
  if (state->isSolenoidOn) {
    lv_label_set_text_fmt(solenoid2Label, "ON");
  } else {
    lv_label_set_text_fmt(solenoid2Label, "OFF");
  }

  lv_obj_t* lastBrewTime1Label = lv_label_create(panel1);
  lv_label_set_text(lastBrewTime1Label, "Last Brew Time:");

  lastBrewTimeLabel = lv_label_create(panel1);
  lv_label_set_text_fmt(lastBrewTimeLabel, "%.2f", state->lastBrewTime);

  static lv_coord_t grid_panel1_col_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 20, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 20, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_panel1_row_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(panel1, grid_panel1_col_dsc, grid_panel1_row_dsc);

  lv_obj_set_grid_cell(tempSet1Label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(tempSet2Label, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(tempRead1Label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(tempRead2Label, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(solenoid1Label, LV_GRID_ALIGN_CENTER, 8, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(solenoid2Label, LV_GRID_ALIGN_CENTER, 10, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(pressureSet1Label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(pressureSet2Label, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(pressureRead1Label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(pressureRead2Label, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(lastBrewTime1Label, LV_GRID_ALIGN_CENTER, 8, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  lv_obj_set_grid_cell(lastBrewTimeLabel, LV_GRID_ALIGN_CENTER, 10, 1, LV_GRID_ALIGN_CENTER, 1, 1);


  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

  lv_obj_set_grid_cell(boilerBtn, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(brewBtn, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(steamBtn, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 5, LV_GRID_ALIGN_STRETCH, 1, 1);
}

static void settings_create(lv_obj_t* parent) {

  int textFieldWidth = 100;

  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

  lv_obj_t* panel1 = lv_obj_create(parent);
  lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  lv_obj_t* brew_temp_label = lv_label_create(panel1);
  lv_label_set_text(brew_temp_label, "Brew Temperature:");

  lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

  brew_temp_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(brew_temp_tf, true);
  lv_obj_set_width(brew_temp_tf, textFieldWidth);
  char t[100];
  sprintf(t, "%.2f", state->boilerSetPoint);
  lv_textarea_set_text(brew_temp_tf, t);
  lv_obj_add_event_cb(brew_temp_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* brew_pressure_label = lv_label_create(panel1);
  lv_label_set_text(brew_pressure_label, "Brew Pressure:");

  brew_pressure_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(brew_pressure_tf, true);
  lv_obj_set_width(brew_pressure_tf, textFieldWidth);
  sprintf(t, "%.2f", state->pressureSetPoint);
  lv_textarea_set_text(brew_pressure_tf, t);
  lv_obj_add_event_cb(brew_pressure_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* steam_temp_label = lv_label_create(panel1);
  lv_label_set_text(steam_temp_label, "Steam Temperature:");

  steam_temp_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(steam_temp_tf, true);
  lv_obj_set_width(steam_temp_tf, textFieldWidth);
  sprintf(t, "%.2f", state->steamSetPoint);
  lv_textarea_set_text(steam_temp_tf, t);
  lv_obj_add_event_cb(steam_temp_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* steam_max_pressure_labl = lv_label_create(panel1);
  lv_label_set_text(steam_max_pressure_labl, "Steam Max Pressure:");

  steam_max_pressure_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(steam_max_pressure_tf, true);
  lv_obj_set_width(steam_max_pressure_tf, textFieldWidth);
  sprintf(t, "%.2f", state->steam_max_pressure);
  lv_textarea_set_text(steam_max_pressure_tf, t);
  lv_obj_add_event_cb(steam_max_pressure_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* steam_pump_output_perc_label = lv_label_create(panel1);
  lv_label_set_text(steam_pump_output_perc_label, "Steam Pump %:");

  steam_pump_output_perc_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(steam_pump_output_perc_tf, true);
  lv_obj_set_width(steam_pump_output_perc_tf, textFieldWidth);
  sprintf(t, "%.2f", state->steam_pump_output_percent);
  lv_textarea_set_text(steam_pump_output_perc_tf, t);
  lv_obj_add_event_cb(steam_pump_output_perc_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* blooming_pressure_label = lv_label_create(panel1);
  lv_label_set_text(blooming_pressure_label, "blooming_pressure:");

  blooming_pressure_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(blooming_pressure_tf, true);
  lv_obj_set_width(blooming_pressure_tf, textFieldWidth);
  sprintf(t, "%.2f", state->blooming_pressure);
  lv_textarea_set_text(blooming_pressure_tf, t);
  lv_obj_add_event_cb(blooming_pressure_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* blooming_fill_time_label = lv_label_create(panel1);
  lv_label_set_text(blooming_fill_time_label, "blooming_fill_time:");

  blooming_fill_time_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(blooming_fill_time_tf, true);
  lv_obj_set_width(blooming_fill_time_tf, textFieldWidth);
  sprintf(t, "%.2f", state->blooming_fill_time);
  lv_textarea_set_text(blooming_fill_time_tf, t);
  lv_obj_add_event_cb(blooming_fill_time_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* blooming_wait_time_label = lv_label_create(panel1);
  lv_label_set_text(blooming_wait_time_label, "blooming_wait_time:");

  blooming_wait_time_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(blooming_wait_time_tf, true);
  lv_obj_set_width(blooming_wait_time_tf, textFieldWidth);
  sprintf(t, "%.2f", state->blooming_wait_time);
  lv_textarea_set_text(blooming_wait_time_tf, t);
  lv_obj_add_event_cb(blooming_wait_time_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* brew_timer_label = lv_label_create(panel1);
  lv_label_set_text(brew_timer_label, "brew_timer:");

  brew_timer_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(brew_timer_tf, true);
  lv_obj_set_width(brew_timer_tf, textFieldWidth);
  sprintf(t, "%.2f", state->brew_timer);
  lv_textarea_set_text(brew_timer_tf, t);
  lv_obj_add_event_cb(brew_timer_tf, setting_field_changed, LV_EVENT_ALL, kb);

  setBtn = lv_btn_create(panel1);
  lv_obj_t* setBtn_label = lv_label_create(setBtn);
  lv_label_set_text(setBtn_label, "Set");
  lv_obj_add_state(setBtn, LV_STATE_DISABLED);
  lv_obj_add_event_cb(setBtn, setButtonClicked, LV_EVENT_ALL, kb);

  lv_obj_t* cancelBtn = lv_btn_create(panel1);
  lv_obj_t* cancelBtn_Label = lv_label_create(cancelBtn);
  lv_obj_set_width(cancelBtn, textFieldWidth);
  lv_label_set_text(cancelBtn_Label, "Cancel");
  lv_obj_add_event_cb(cancelBtn_Label, cancelButtonClicked, LV_EVENT_ALL, kb);


  cleanBtn = lv_btn_create(panel1);
  lv_obj_t* cleanBtn_label = lv_label_create(cleanBtn);
  lv_label_set_text(cleanBtn_label, "Clean");
  lv_obj_add_event_cb(cleanBtn, cleanBtnClicked, LV_EVENT_ALL, kb);

  clearLogsBtn = lv_btn_create(panel1);
  lv_obj_t* clearLogsBtn_label = lv_label_create(clearLogsBtn);
  lv_label_set_text(clearLogsBtn_label, "Clear Logs");
  lv_obj_add_event_cb(clearLogsBtn, clearLogsBtnClicked, LV_EVENT_ALL, kb);

  static lv_coord_t grid_panel1_col_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_panel1_row_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(panel1, grid_panel1_col_dsc, grid_panel1_row_dsc);

  lv_obj_set_grid_cell(brew_temp_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(brew_temp_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(brew_pressure_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(brew_pressure_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(steam_temp_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(steam_temp_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(steam_max_pressure_labl, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_obj_set_grid_cell(steam_max_pressure_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_obj_set_grid_cell(brew_timer_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 8, 1);
  lv_obj_set_grid_cell(brew_timer_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 8, 1);

  lv_obj_set_grid_cell(setBtn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 10, 1);
  lv_obj_set_grid_cell(cancelBtn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 10, 1);

  lv_obj_set_grid_cell(steam_pump_output_perc_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(steam_pump_output_perc_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(blooming_pressure_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(blooming_pressure_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(blooming_fill_time_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(blooming_fill_time_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(blooming_wait_time_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_obj_set_grid_cell(blooming_wait_time_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 6, 1);

  lv_obj_set_grid_cell(cleanBtn, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 10, 1);
  lv_obj_set_grid_cell(clearLogsBtn, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 10, 1);
}


static void advancedSettings_create(lv_obj_t* parent) {
  int textFieldWidth = 100;

  lv_obj_t* panel1 = lv_obj_create(parent);

  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);
  lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  lv_obj_t* boiler_bb_range_label = lv_label_create(panel1);
  lv_label_set_text(boiler_bb_range_label, "boiler_bb_range:");

  lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

  boiler_bb_range_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(boiler_bb_range_tf, true);
  lv_obj_set_width(boiler_bb_range_tf, textFieldWidth);
  char t[100];
  sprintf(t, "%.2f", advancedSettings->boiler_bb_range);
  lv_textarea_set_text(boiler_bb_range_tf, t);
  lv_obj_add_event_cb(boiler_bb_range_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* boiler_PID_cicle_label = lv_label_create(panel1);
  lv_label_set_text(boiler_PID_cicle_label, "boiler_PID_cicle:");

  boiler_PID_cycle_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(boiler_PID_cycle_tf, true);
  lv_obj_set_width(boiler_PID_cycle_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->boiler_PID_cycle);
  lv_textarea_set_text(boiler_PID_cycle_tf, t);
  lv_obj_add_event_cb(boiler_PID_cycle_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* boiler_PID_KP_label = lv_label_create(panel1);
  lv_label_set_text(boiler_PID_KP_label, "boiler_PID_KP:");

  boiler_PID_KP_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(boiler_PID_KP_tf, true);
  lv_obj_set_width(boiler_PID_KP_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->boiler_PID_KP);
  lv_textarea_set_text(boiler_PID_KP_tf, t);
  lv_obj_add_event_cb(boiler_PID_KP_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* boiler_PID_KI_label = lv_label_create(panel1);
  lv_label_set_text(boiler_PID_KI_label, "boiler_PID_KI:");

  boiler_PID_KI_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(boiler_PID_KI_tf, true);
  lv_obj_set_width(boiler_PID_KI_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->boiler_PID_KI);
  lv_textarea_set_text(boiler_PID_KI_tf, t);
  lv_obj_add_event_cb(boiler_PID_KI_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* boiler_PID_KD_label = lv_label_create(panel1);
  lv_label_set_text(boiler_PID_KD_label, "boiler_PID_KD:");

  boiler_PID_KD_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(boiler_PID_KD_tf, true);
  lv_obj_set_width(boiler_PID_KD_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->boiler_PID_KD);
  lv_textarea_set_text(boiler_PID_KD_tf, t);
  lv_obj_add_event_cb(boiler_PID_KD_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* pump_max_step_up_label = lv_label_create(panel1);
  lv_label_set_text(pump_max_step_up_label, "pump_max_step_up:");

  pump_max_step_up_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(pump_max_step_up_tf, true);
  lv_obj_set_width(pump_max_step_up_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->pump_max_step_up);
  lv_textarea_set_text(pump_max_step_up_tf, t);
  lv_obj_add_event_cb(pump_max_step_up_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* pump_KP_label = lv_label_create(panel1);
  lv_label_set_text(pump_KP_label, "pump_KP:");

  pump_KP_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(pump_KP_tf, true);
  lv_obj_set_width(pump_KP_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->pump_KP);
  lv_textarea_set_text(pump_KP_tf, t);
  lv_obj_add_event_cb(pump_KP_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* pump_KI_label = lv_label_create(panel1);
  lv_label_set_text(pump_KI_label, "pump_KI:");

  pump_KI_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(pump_KI_tf, true);
  lv_obj_set_width(pump_KI_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->pump_KI);
  lv_textarea_set_text(pump_KI_tf, t);
  lv_obj_add_event_cb(pump_KI_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* pump_KD_label = lv_label_create(panel1);
  lv_label_set_text(pump_KD_label, "pump_KD:");

  pump_KD_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(pump_KD_tf, true);
  lv_obj_set_width(pump_KD_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->pump_KD);
  lv_textarea_set_text(pump_KD_tf, t);
  lv_obj_add_event_cb(pump_KD_tf, setting_field_changed, LV_EVENT_ALL, kb);

  lv_obj_t* unused1_label = lv_label_create(panel1);
  lv_label_set_text(unused1_label, "unused1:");

  unused1_tf = lv_textarea_create(panel1);
  lv_textarea_set_one_line(unused1_tf, true);
  lv_obj_set_width(unused1_tf, textFieldWidth);
  sprintf(t, "%.2f", advancedSettings->unused1);
  lv_textarea_set_text(unused1_tf, t);
  lv_obj_add_event_cb(unused1_tf, setting_field_changed, LV_EVENT_ALL, kb);

  advancedSetBtn = lv_btn_create(panel1);
  lv_obj_t* advancedSetBtn_label = lv_label_create(advancedSetBtn);
  lv_label_set_text(advancedSetBtn_label, "Set");
  lv_obj_add_state(advancedSetBtn, LV_STATE_DISABLED);
  lv_obj_add_event_cb(advancedSetBtn, advancedSetButtonClicked, LV_EVENT_ALL, kb);

  lv_obj_t* advancedCancelBtn = lv_btn_create(panel1);
  lv_obj_t* advancedCancelBtn_Label = lv_label_create(advancedCancelBtn);
  lv_label_set_text(advancedCancelBtn_Label, "Cancel");
  lv_obj_add_event_cb(advancedCancelBtn_Label, advancedCancelButtonClicked, LV_EVENT_ALL, kb);

  static lv_coord_t grid_panel1_col_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 10, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_panel1_row_dsc[] = { LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(panel1, grid_panel1_col_dsc, grid_panel1_row_dsc);

  lv_obj_set_grid_cell(boiler_bb_range_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(boiler_bb_range_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(boiler_PID_cicle_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(boiler_PID_cycle_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(boiler_PID_KP_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(boiler_PID_KP_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(boiler_PID_KI_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_obj_set_grid_cell(boiler_PID_KI_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_obj_set_grid_cell(boiler_PID_KD_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 8, 1);
  lv_obj_set_grid_cell(boiler_PID_KD_tf, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 8, 1);

  lv_obj_set_grid_cell(pump_max_step_up_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(pump_max_step_up_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(pump_KP_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(pump_KP_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(pump_KI_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(pump_KI_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(pump_KD_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_obj_set_grid_cell(pump_KD_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_obj_set_grid_cell(unused1_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 8, 1);
  lv_obj_set_grid_cell(unused1_tf, LV_GRID_ALIGN_CENTER, 6, 1, LV_GRID_ALIGN_CENTER, 8, 1);

  lv_obj_set_grid_cell(advancedSetBtn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 10, 1);
  lv_obj_set_grid_cell(advancedCancelBtn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 10, 1);
}

// #endif
