/**
 * @file lv_buildUI.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_buildUI.h"
#include "my_logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/*Create an AZERTY keyboard map*/
static const char* kb_num_map[] = { "1", "2", "3", LV_SYMBOL_BACKSPACE, "\n",
                                    "4", "5", "6", " ", "\n",
                                    "7", "8", "9", ".", "\n",
                                    "0", LV_SYMBOL_CLOSE, LV_SYMBOL_OK, NULL };

static const lv_btnmatrix_ctrl_t kb_num_ctrl[] = { 1, 1, 1, 1,
                                                   1, 1, 1, 1,
                                                   1, 1, 1, 1,
                                                   2, 2, 2 };

static int (*writeConfigFile)();
static int (*listProfiles)(char* buf, size_t size);
static int (*getCurrentProfile)(char* buf, size_t size);
static int (*writeCurrentProfile)(const char* profileName);
static int (*setupAndReadConfigFile)();
static int (*renameProfile)(const char* newName);
static bool (*deleteProfile)(const char* profileToDelete);
static int (*duplicateProfile)();

static void basic_create(lv_obj_t* parent);
static void settings_create(lv_obj_t* parent);
static void advancedSettings_create(lv_obj_t* parent);
static void profile_create(lv_obj_t* parent);
static void main_create(lv_obj_t* parent);
static void updateProfileTab();
void updateSettings();

/**********************
 *  STATIC VARIABLES
 **********************/
static GaggiaStateT* state;
static AdvancedSettingsT* advancedSettings;

static lv_obj_t* tabMain;
static lv_obj_t* tabProfile;
static lv_obj_t* tabSettings;
static lv_obj_t* tabAdvance;
static lv_obj_t* tabBrew;

static lv_obj_t* selectedProfileLabel;

static lv_obj_t* heat_btn;
static lv_obj_t* heat_btn_label;
static lv_obj_t* boil_btn;
static lv_obj_t* boil_btn_label;
static lv_obj_t* brew_btn;
static lv_obj_t* brew_btn_label;
static lv_obj_t* clean_btn;
static lv_obj_t* clean_btn_label;
static lv_obj_t* prime_btn;
static lv_obj_t* prime_btn_label;
static lv_obj_t* auto_btn;
static lv_obj_t* auto_btn_label;
static lv_obj_t* temp_label;
static lv_obj_t* press_label;
static lv_obj_t* main_notes_label;
static lv_obj_t* time_label;

static lv_obj_t* notes_tf;
static lv_obj_t* fileList;
static lv_obj_t* fileName_tf;
static lv_obj_t* fileName_btn;
static lv_obj_t* duplicate_btn;
static lv_obj_t* delete_btn;

static lv_obj_t* edit_notes_tf;
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

static const lv_font_t* font_large;
static const lv_font_t* font_normal;
static lv_obj_t* tv;
static lv_obj_t* calendar;
static lv_style_t style_text_muted;
static lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;

/**********************
 *      UTILS
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
    // LV_LOG_WARN("Setting changed");
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
    strncpy(state->notes, lv_textarea_get_text(edit_notes_tf), NOTES_MAX - 1);
    state->notes[NOTES_MAX - 1] = '\0';
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

// static void cleanBtnClicked(lv_event_t* e) {
//   lv_event_code_t code = lv_event_get_code(e);

//   if (code == LV_EVENT_CLICKED) {
//     LV_LOG_WARN("Clean Button Clicked");
//     if (state->isBrewing) {
//       LV_LOG_USER("no cleaning while brewing");
//     } else {
//       state->isCleaning = true;
//       state->hasCommandChanged = true;
//     }
//   }
// }
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
      lv_label_set_text(selectedProfileLabel, clickedProfileName);
      writeCurrentProfile(clickedProfileName);
      setupAndReadConfigFile();
      //TODO shouldn't we update the settings/advanced tab?
    }
  }
}

void updateProfileTab() {
  char names[512];
  if (listProfiles(names, sizeof(names)) < 0) names[0] = '\0';
  LV_LOG_WARN(names);
  int index = 0;
  char* start = names;
  while (*start != '\0' && index < 10) {
    char* end = strchr(start, ';');
    if (end == NULL) break;
    *end = '\0';
    lv_obj_t* child = lv_obj_get_child(fileList, index);
    lv_label_set_text(child, start);
    start = end + 1;
    index++;
  }
  for (; index < 10; index++) {
    lv_obj_t* child = lv_obj_get_child(fileList, index);
    lv_label_set_text(child, "");
  }

  char fn[PROFILE_NAME_MAX];
  if (getCurrentProfile(fn, sizeof(fn)) < 0) fn[0] = '\0';
  lv_textarea_set_text(fileName_tf, fn);
  lv_label_set_text(selectedProfileLabel, fn);
}

static void fileName_btn_clicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("rename Clicked");
    const char* newProfileName = lv_textarea_get_text(fileName_tf);
    if (strlen(newProfileName) > 0) {
      renameProfile(newProfileName);
      updateProfileTab();
    }
  }
}
static void duplicate_btn_clicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("duplicate Clicked");
    duplicateProfile();
    updateProfileTab();
  }
}
static void delete_btn_clicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("delete Clicked");
    const char* profileToDelete = lv_textarea_get_text(fileName_tf);
    if (strlen(profileToDelete) > 0) {
      LV_LOG_WARN("delete this profile");
      LV_LOG_WARN(profileToDelete);
      deleteProfile(profileToDelete);
      LV_LOG_WARN("profile deleted, now selecting default");

      //select new default
      lv_obj_t* child = lv_obj_get_child(fileList, 0);
      LV_LOG_WARN("picked first in the list");
      const char* newProfileName = lv_label_get_text(child);
      if (strlen(newProfileName) > 0) {
        LV_LOG_WARN("new selected profile will be");
        LV_LOG_WARN("newProfileName");
        writeCurrentProfile(newProfileName);
        setupAndReadConfigFile();
      }
      updateProfileTab();
    }
  }
}

void disableTabs() {
  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tv);
  lv_obj_add_state(tab_btns, LV_STATE_DISABLED);
}

void enableTabs() {
  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tv);
  lv_obj_clear_state(tab_btns, LV_STATE_DISABLED);
}

static void main_btn_clicked(lv_event_t* e) {
  lv_obj_t* target = lv_event_get_target(e);
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    if (target == heat_btn) {
      LV_LOG_USER("heat_btn button clicked");
      if (lv_obj_get_state(heat_btn) & LV_STATE_CHECKED) {
        if (lv_obj_get_state(boil_btn) & LV_STATE_CHECKED) {
          LV_LOG_USER("stoping steam");
          lv_obj_clear_state(boil_btn, LV_STATE_CHECKED);
        }
        disableTabs();
        LV_LOG_USER("starting heat");
        state->isBoilerOn = true;
        state->isSteaming = false;
        state->hasCommandChanged = true;
      } else {
        enableTabs();
        LV_LOG_USER("stoping heat");
        state->isBoilerOn = false;
        state->isSteaming = false;
        state->hasCommandChanged = true;
      }
    } else if (target == boil_btn) {
      LV_LOG_USER("boil_btn button clicked");
      if (lv_obj_get_state(boil_btn) & LV_STATE_CHECKED) {
        if (lv_obj_get_state(heat_btn) & LV_STATE_CHECKED) {
          LV_LOG_USER("stoping heat");
          lv_obj_clear_state(heat_btn, LV_STATE_CHECKED);
        }
        disableTabs();
        LV_LOG_USER("starting boil");
        state->isBoilerOn = false;
        state->isSteaming = true;
        state->hasCommandChanged = true;
      } else {
        enableTabs();
        LV_LOG_USER("stoping boil");
        state->isBoilerOn = false;
        state->isSteaming = false;
        state->hasCommandChanged = true;
      }
    } else if (target == brew_btn) {
      LV_LOG_USER("brew_btn button clicked");
      if (lv_obj_get_state(brew_btn) & LV_STATE_CHECKED) {
        //disable 3 other buttons
        lv_obj_add_state(clean_btn, LV_STATE_DISABLED);
        lv_obj_add_state(prime_btn, LV_STATE_DISABLED);
        lv_obj_add_state(auto_btn, LV_STATE_DISABLED);
        disableTabs();
        LV_LOG_USER("starting brew");
        state->isBrewing = true;
        state->hasCommandChanged = true;
      } else {
        //reenable 3 other buttons
        lv_obj_clear_state(clean_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(prime_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(auto_btn, LV_STATE_DISABLED);
        enableTabs();
        LV_LOG_USER("stoping brew");
        state->isBrewing = false;
        state->hasCommandChanged = true;
      }
    } else if (target == clean_btn) {
      LV_LOG_USER("clean_btn button clicked");
      if (lv_obj_get_state(clean_btn) & LV_STATE_CHECKED) {
        //disable 3 other buttons
        lv_obj_add_state(brew_btn, LV_STATE_DISABLED);
        lv_obj_add_state(prime_btn, LV_STATE_DISABLED);
        lv_obj_add_state(auto_btn, LV_STATE_DISABLED);
        disableTabs();
        LV_LOG_USER("starting clean");
        state->isCleaning = true;
        state->hasCommandChanged = true;
      } else {
        //reenable 3 other buttons
        lv_obj_clear_state(brew_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(prime_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(auto_btn, LV_STATE_DISABLED);
        enableTabs();
        LV_LOG_USER("stoping clean");
        state->isCleaning = false;
        state->hasCommandChanged = true;
      }
    } else if (target == prime_btn) {
      LV_LOG_USER("prime_btn button clicked");
      if (lv_obj_get_state(prime_btn) & LV_STATE_CHECKED) {
        //disable 3 other buttons
        lv_obj_add_state(clean_btn, LV_STATE_DISABLED);
        lv_obj_add_state(brew_btn, LV_STATE_DISABLED);
        lv_obj_add_state(auto_btn, LV_STATE_DISABLED);
        disableTabs();
        LV_LOG_USER("starting bloom");
        state->isBlooming = true;
        state->hasCommandChanged = true;
      } else {
        //reenable 3 other buttons
        lv_obj_clear_state(clean_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(brew_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(auto_btn, LV_STATE_DISABLED);
        enableTabs();
        LV_LOG_USER("stoping bloom");
        state->isBlooming = false;
        state->hasCommandChanged = true;
      }
    } else if (target == auto_btn) {
      LV_LOG_USER("auto_btn button clicked");
      if (lv_obj_get_state(auto_btn) & LV_STATE_CHECKED) {
        //disable 3 other buttons
        lv_obj_add_state(clean_btn, LV_STATE_DISABLED);
        lv_obj_add_state(prime_btn, LV_STATE_DISABLED);
        lv_obj_add_state(brew_btn, LV_STATE_DISABLED);
        disableTabs();
        LV_LOG_USER("starting auto");
        state->isAuto = true;
        state->hasCommandChanged = true;
      } else {
        //reenable 3 other buttons
        lv_obj_clear_state(clean_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(prime_btn, LV_STATE_DISABLED);
        lv_obj_clear_state(brew_btn, LV_STATE_DISABLED);
        enableTabs();
        LV_LOG_USER("stoping auto");
        state->isAuto = false;
        state->hasCommandChanged = true;
      }
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
  LV_LOG_TRACE("updating UI");
  if (state->hasConfigChanged) {
    lv_label_set_text_fmt(lv_obj_get_child(heat_btn, 0), "H:%.0fC", state->boilerSetPoint);
    lv_label_set_text_fmt(lv_obj_get_child(boil_btn, 0), "S:%.0fC", state->steamSetPoint);
    lv_label_set_text_fmt(lv_obj_get_child(brew_btn, 0), "B:%.1fb", state->pressureSetPoint);
    lv_label_set_text_fmt(lv_obj_get_child(prime_btn, 0), "P:%.0f+%.0fs", state->blooming_fill_time, state->blooming_wait_time);  // fill + wait
    lv_label_set_text_fmt(lv_obj_get_child(auto_btn, 0), "A:%.0fs", state->brew_timer);
    lv_label_set_text(selectedProfileLabel, state->profile_name);
  }
  lv_label_set_text_fmt(temp_label, "%.0fC", state->tempRead);
  lv_label_set_text_fmt(press_label, "%.1fb", state->pressureRead);
  // Bloom and auto end on their own (sequencer): release the buttons and the tabs
  // the way a manual stop does.
  if (state->isBlooming == false) {
    if (lv_obj_has_state(prime_btn, LV_STATE_CHECKED)) {
      lv_obj_clear_state(prime_btn, LV_STATE_CHECKED);
      lv_obj_clear_state(clean_btn, LV_STATE_DISABLED);
      lv_obj_clear_state(brew_btn, LV_STATE_DISABLED);
      lv_obj_clear_state(auto_btn, LV_STATE_DISABLED);
      enableTabs();
    }
  }
  if (state->isAuto == false) {
    if (lv_obj_has_state(auto_btn, LV_STATE_CHECKED)) {
      lv_obj_clear_state(auto_btn, LV_STATE_CHECKED);
      lv_obj_clear_state(clean_btn, LV_STATE_DISABLED);
      lv_obj_clear_state(brew_btn, LV_STATE_DISABLED);
      lv_obj_clear_state(prime_btn, LV_STATE_DISABLED);
      enableTabs();
    }
  }
  if (state->actionStartTime > 0) {
    if (state->actionStopTime > 0) {
      lv_label_set_text_fmt(time_label, "%ds", (state->actionStopTime - state->actionStartTime) / 1000);
    } else {
      lv_label_set_text_fmt(time_label, "%ds", (millis() - state->actionStartTime) / 1000);
    }
  } else {
    lv_label_set_text_fmt(time_label, "%ds", 0);
  }
  updateSettings();
}

void updateSettings() {
  LV_LOG_TRACE("updating settings fields");

  if (state->hasConfigChanged) {
    // LV_LOG_WARN("updating config fields");

    char t[100];
    lv_textarea_set_text(notes_tf, state->notes);
    lv_label_set_text(main_notes_label, state->notes);
    lv_textarea_set_text(edit_notes_tf, state->notes);

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
    // LV_LOG_WARN("updating advanced Settings fields");

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
                   int (*lp)(char* buf, size_t size),
                   int (*gcp)(char* buf, size_t size),
                   int (*wcp)(const char* profileName),
                   int (*sarcf)(),
                   int (*rp)(const char* newName),
                   bool (*dp)(const char* profileToDelete),
                   int (*dupp)()) {
  state = s;
  advancedSettings = as;
  writeConfigFile = f;
  listProfiles = lp;
  getCurrentProfile = gcp;
  writeCurrentProfile = wcp;
  setupAndReadConfigFile = sarcf;
  renameProfile = rp;
  deleteProfile = dp;
  duplicateProfile = dupp;

  lv_log_register_print_cb(my_log_cb);

  LV_LOG_ERROR("logging works!!");

  font_large = LV_FONT_DEFAULT;
  font_normal = LV_FONT_DEFAULT;

  lv_coord_t tab_h;

  tab_h = 70;

  // defined in ../libraries/lv_conf.h
#if LV_FONT_MONTSERRAT_36
  font_large = &lv_font_montserrat_36;
#else
  LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_24
  font_normal = &lv_font_montserrat_24;
#else
  LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
  // #if LV_FONT_MONTSERRAT_26
  //   font_normal = &lv_font_montserrat_26;
  // #else
  //   LV_LOG_WARN("LV_FONT_MONTSERRAT_20 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
  // #endif


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
  lv_obj_add_event_cb(tv, tab_changed, LV_EVENT_ALL, NULL);

  lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);


  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tv);
  lv_obj_set_style_pad_left(tab_btns, LV_HOR_RES / 3, 0);

  // lv_obj_t* logo = lv_img_create(tab_btns);
  // LV_IMG_DECLARE(logoMainScreen);
  // lv_img_set_src(logo, &logoMainScreen);
  // lv_obj_align(logo, LV_ALIGN_LEFT_MID, -LV_HOR_RES / 2 + 25, 0);

  selectedProfileLabel = lv_label_create(tab_btns);
  // lv_obj_align(selectedProfileLabel,LV_ALIGN_RIGHT_MID,-LV_HOR_RES / 2, 0);
  lv_obj_align(selectedProfileLabel, LV_ALIGN_RIGHT_MID, ((-2 * LV_HOR_RES) / 3) - 20, 0);
  lv_obj_set_style_text_font(selectedProfileLabel, font_large, 0);

  tabMain = lv_tabview_add_tab(tv, "Main");
  tabProfile = lv_tabview_add_tab(tv, "Prof.");
  tabSettings = lv_tabview_add_tab(tv, "Sett.");
  tabAdvance = lv_tabview_add_tab(tv, "Adv.");

  lv_obj_set_style_text_font(tabMain, font_large, 0);

  main_create(tabMain);
  profile_create(tabProfile);
  settings_create(tabSettings);
  advancedSettings_create(tabAdvance);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void main_create(lv_obj_t* parent) {

  lv_obj_t* panel1 = lv_obj_create(parent);
  lv_obj_set_height(panel1, LV_SIZE_CONTENT);

  heat_btn = lv_btn_create(parent);
  lv_obj_add_flag(heat_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(heat_btn, main_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* heat_btn_label = lv_label_create(heat_btn);
  lv_label_set_text_fmt(heat_btn_label, "H:%.0fC", state->boilerSetPoint);
  lv_obj_center(heat_btn_label);

  boil_btn = lv_btn_create(parent);
  lv_obj_add_flag(boil_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(boil_btn, main_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* boil_btn_label = lv_label_create(boil_btn);
  lv_label_set_text_fmt(boil_btn_label, "S:%.0fC", state->steamSetPoint);
  lv_obj_center(boil_btn_label);

  brew_btn = lv_btn_create(parent);
  lv_obj_add_flag(brew_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(brew_btn, main_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* brew_btn_label = lv_label_create(brew_btn);
  lv_label_set_text_fmt(brew_btn_label, "B:%.1fb", state->pressureSetPoint);
  lv_obj_center(brew_btn_label);

  clean_btn = lv_btn_create(parent);
  lv_obj_add_flag(clean_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(clean_btn, main_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* clean_btn_label = lv_label_create(clean_btn);
  lv_label_set_text(clean_btn_label, "Clean");
  lv_obj_center(clean_btn_label);

  prime_btn = lv_btn_create(parent);
  lv_obj_add_flag(prime_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(prime_btn, main_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* prime_btn_label = lv_label_create(prime_btn);
  lv_label_set_text_fmt(prime_btn_label, "P:%.0f+%.0fs", state->blooming_fill_time, state->blooming_wait_time);
  lv_obj_center(prime_btn_label);

  auto_btn = lv_btn_create(parent);
  lv_obj_add_flag(auto_btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(auto_btn, main_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* auto_btn_label = lv_label_create(auto_btn);
  lv_label_set_text_fmt(auto_btn_label, "A:%.0fs", state->brew_timer);
  lv_obj_center(auto_btn_label);

  main_notes_label = lv_label_create(panel1);
  lv_obj_set_style_text_color(main_notes_label, lv_color_hex(0x00AAFF), 0);

  temp_label = lv_label_create(panel1);
  lv_label_set_text_fmt(temp_label, "%.0fC", state->tempRead);
  lv_obj_center(temp_label);

  press_label = lv_label_create(panel1);
  lv_label_set_text_fmt(press_label, "%.1fb", state->pressureRead);
  lv_obj_center(press_label);

  time_label = lv_label_create(panel1);
  lv_label_set_text_fmt(time_label, "%ds", 0);
  lv_obj_center(time_label);


  static lv_coord_t grid_panel1_col_dsc[] = { LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_panel1_row_dsc[] = { LV_GRID_FR(1), 10,  LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(panel1, grid_panel1_col_dsc, grid_panel1_row_dsc);

  lv_obj_set_grid_cell(temp_label, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(press_label, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(time_label, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
  lv_obj_set_grid_cell(main_notes_label, LV_GRID_ALIGN_CENTER, 0, 5, LV_GRID_ALIGN_CENTER, 2, 1);


  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

  lv_obj_set_grid_cell(heat_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(brew_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(prime_btn, LV_GRID_ALIGN_STRETCH, 4, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(boil_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(clean_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(auto_btn, LV_GRID_ALIGN_STRETCH, 4, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 5, LV_GRID_ALIGN_STRETCH, 4, 1);
}

static void profile_create(lv_obj_t* parent) {

  notes_tf = lv_textarea_create(parent);
  lv_textarea_set_one_line(notes_tf, true);

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
  lv_textarea_set_max_length(fileName_tf, PROFILE_NAME_MAX - 1);
  lv_textarea_set_one_line(fileName_tf, true);
  lv_obj_add_event_cb(fileName_tf, setting_field_changed, LV_EVENT_ALL, kb);

  fileName_btn = lv_btn_create(parent);
  lv_obj_add_event_cb(fileName_btn, fileName_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* fileName_btn_lbl = lv_label_create(fileName_btn);
  lv_label_set_text(fileName_btn_lbl, "Rename");
  lv_obj_center(fileName_btn_lbl);

  duplicate_btn = lv_btn_create(parent);
  lv_obj_add_event_cb(duplicate_btn, duplicate_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* duplicate_btn_lbl = lv_label_create(duplicate_btn);
  lv_label_set_text(duplicate_btn_lbl, "Duplicate");
  lv_obj_center(duplicate_btn_lbl);

  delete_btn = lv_btn_create(parent);
  lv_obj_add_event_cb(delete_btn, delete_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_t* delete_btn_lbl = lv_label_create(delete_btn);
  lv_label_set_text(delete_btn_lbl, "Delete");
  lv_obj_center(delete_btn_lbl);

  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), 5, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), 1, LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), 10, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

  lv_obj_set_grid_cell(notes_tf, LV_GRID_ALIGN_STRETCH, 0, 3, LV_GRID_ALIGN_STRETCH, 0, 1);

  lv_obj_set_grid_cell(fileList, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 7);

  lv_obj_set_grid_cell(fileName_tf, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, 2, 1);
  lv_obj_set_grid_cell(fileName_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, 4, 1);
  lv_obj_set_grid_cell(duplicate_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, 6, 1);
  lv_obj_set_grid_cell(delete_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, 8, 1);
}

static void settings_create(lv_obj_t* parent) {

  int textFieldWidth = 300;

  static lv_coord_t grid_main_col_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t grid_main_row_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

  lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

  lv_obj_t* panel1 = lv_obj_create(parent);
  lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_flex_flow(panel1, LV_FLEX_FLOW_COLUMN);

  lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kb_num_map, kb_num_ctrl);
  lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
  //--------------

  lv_obj_t* kb2 = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb2, LV_OBJ_FLAG_HIDDEN);

  edit_notes_tf = lv_textarea_create(panel1);
  lv_textarea_set_max_length(edit_notes_tf, NOTES_MAX - 1);
  lv_textarea_set_one_line(edit_notes_tf, true);
  lv_obj_set_size(edit_notes_tf, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_add_event_cb(edit_notes_tf, setting_field_changed, LV_EVENT_ALL, kb2);
  //--------------
  lv_obj_t* sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* brew_temp_label = lv_label_create(sub_panel);
  lv_obj_set_size(brew_temp_label, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(brew_temp_label, "Brew Temperature:");

  brew_temp_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(brew_temp_tf, true);
  lv_obj_set_size(brew_temp_tf, textFieldWidth, LV_SIZE_CONTENT);
  char t[100];
  sprintf(t, "%.2f", state->boilerSetPoint);
  lv_textarea_set_text(brew_temp_tf, t);
  lv_obj_add_event_cb(brew_temp_tf, setting_field_changed, LV_EVENT_ALL, kb);
  //--------------
  sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* brew_pressure_label = lv_label_create(sub_panel);
  lv_obj_set_size(brew_pressure_label, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(brew_pressure_label, "Brew Pressure:");

  brew_pressure_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(brew_pressure_tf, true);
  lv_obj_set_size(brew_pressure_tf, textFieldWidth, LV_SIZE_CONTENT);
  sprintf(t, "%.2f", state->pressureSetPoint);
  lv_textarea_set_text(brew_pressure_tf, t);
  lv_obj_add_event_cb(brew_pressure_tf, setting_field_changed, LV_EVENT_ALL, kb);
  //--------------
  sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* steam_temp_label = lv_label_create(sub_panel);
  lv_obj_set_size(steam_temp_label, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(steam_temp_label, "Steam Temperature:");

  steam_temp_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(steam_temp_tf, true);
  lv_obj_set_size(steam_temp_tf, textFieldWidth, LV_SIZE_CONTENT);
  sprintf(t, "%.2f", state->steamSetPoint);
  lv_textarea_set_text(steam_temp_tf, t);
  lv_obj_add_event_cb(steam_temp_tf, setting_field_changed, LV_EVENT_ALL, kb);

  //--------------
  sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* steam_max_pressure_labl = lv_label_create(sub_panel);
  lv_obj_set_size(steam_max_pressure_labl, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(steam_max_pressure_labl, "Steam Max Pressure:");

  steam_max_pressure_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(steam_max_pressure_tf, true);
  lv_obj_set_size(steam_max_pressure_tf, textFieldWidth, LV_SIZE_CONTENT);
  sprintf(t, "%.2f", state->steam_max_pressure);
  lv_textarea_set_text(steam_max_pressure_tf, t);
  lv_obj_add_event_cb(steam_max_pressure_tf, setting_field_changed, LV_EVENT_ALL, kb);

  //--------------
  sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* steam_pump_output_perc_label = lv_label_create(sub_panel);
  lv_obj_set_size(steam_pump_output_perc_label, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(steam_pump_output_perc_label, "Steam Pump %:");

  steam_pump_output_perc_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(steam_pump_output_perc_tf, true);
  lv_obj_set_size(steam_pump_output_perc_tf, textFieldWidth, LV_SIZE_CONTENT);
  sprintf(t, "%.2f", state->steam_pump_output_percent);
  lv_textarea_set_text(steam_pump_output_perc_tf, t);
  lv_obj_add_event_cb(steam_pump_output_perc_tf, setting_field_changed, LV_EVENT_ALL, kb);

  //--------------
  sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* blooming_pressure_label = lv_label_create(sub_panel);
  lv_obj_set_size(blooming_pressure_label, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(blooming_pressure_label, "blooming_pressure:");

  blooming_pressure_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(blooming_pressure_tf, true);
  lv_obj_set_size(blooming_pressure_tf, textFieldWidth, LV_SIZE_CONTENT);
  sprintf(t, "%.2f", state->blooming_pressure);
  lv_textarea_set_text(blooming_pressure_tf, t);
  lv_obj_add_event_cb(blooming_pressure_tf, setting_field_changed, LV_EVENT_ALL, kb);

  //--------------
  sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* blooming_fill_time_label = lv_label_create(sub_panel);
  lv_obj_set_size(blooming_fill_time_label, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(blooming_fill_time_label, "blooming_fill_time:");

  blooming_fill_time_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(blooming_fill_time_tf, true);
  lv_obj_set_size(blooming_fill_time_tf, textFieldWidth, LV_SIZE_CONTENT);
  sprintf(t, "%.2f", state->blooming_fill_time);
  lv_textarea_set_text(blooming_fill_time_tf, t);
  lv_obj_add_event_cb(blooming_fill_time_tf, setting_field_changed, LV_EVENT_ALL, kb);

  //--------------
  sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* blooming_wait_time_label = lv_label_create(sub_panel);
  lv_obj_set_size(blooming_wait_time_label, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(blooming_wait_time_label, "blooming_wait_time:");

  blooming_wait_time_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(blooming_wait_time_tf, true);
  lv_obj_set_size(blooming_wait_time_tf, textFieldWidth, LV_SIZE_CONTENT);
  sprintf(t, "%.2f", state->blooming_wait_time);
  lv_textarea_set_text(blooming_wait_time_tf, t);
  lv_obj_add_event_cb(blooming_wait_time_tf, setting_field_changed, LV_EVENT_ALL, kb);

  //--------------
  sub_panel = lv_obj_create(panel1);
  lv_obj_set_flex_flow(sub_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(sub_panel, LV_PCT(100), LV_SIZE_CONTENT);

  lv_obj_t* brew_timer_label = lv_label_create(sub_panel);
  lv_obj_set_size(brew_timer_label, textFieldWidth, LV_SIZE_CONTENT);
  lv_label_set_text(brew_timer_label, "brew_timer:");

  brew_timer_tf = lv_textarea_create(sub_panel);
  lv_textarea_set_one_line(brew_timer_tf, true);
  lv_obj_set_size(brew_timer_tf, textFieldWidth, LV_SIZE_CONTENT);
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


  // cleanBtn = lv_btn_create(panel1);
  // lv_obj_t* cleanBtn_label = lv_label_create(cleanBtn);
  // lv_label_set_text(cleanBtn_label, "Clean");
  // lv_obj_add_event_cb(cleanBtn, cleanBtnClicked, LV_EVENT_ALL, kb);

  clearLogsBtn = lv_btn_create(panel1);
  lv_obj_t* clearLogsBtn_label = lv_label_create(clearLogsBtn);
  lv_label_set_text(clearLogsBtn_label, "Clear Logs");
  lv_obj_add_event_cb(clearLogsBtn, clearLogsBtnClicked, LV_EVENT_ALL, kb);
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
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kb_num_map, kb_num_ctrl);
  lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);

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
