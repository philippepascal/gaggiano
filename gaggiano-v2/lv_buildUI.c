/**
 * @file lv_buildUI.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_buildUI.h"
#include "my_logging.h"
#include "theme.h"
#include "history.h"
#include "sequencer.h"
#include "net.h"
#include "timezones.h"
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

static void settings_create(lv_obj_t* parent);
static void advancedSettings_create(lv_obj_t* parent);
static void profile_create(lv_obj_t* parent);
static void main_create(lv_obj_t* parent);
static void graph_create(lv_obj_t* parent);
static void wifi_create(lv_obj_t* parent);
static void wifi_refresh(void);
static void wifi_enter(void);
static void header_net_refresh(void);
static lv_obj_t* group_create(lv_obj_t* column, const char* title);
static lv_obj_t* field_create(lv_obj_t* group, const char* text, lv_obj_t* kb);
static lv_obj_t* column_create(lv_obj_t* parent, int col, int row);
static lv_obj_t* view_btn_create(lv_obj_t* parent, const char* text, lv_event_cb_t cb);
static void editor_kb_event(lv_event_t* e);
static void graph_update(uint32_t now);
static void updateProfileTab();
void updateSettings();

#define HEADER_H 44
#define TILE_H 140
#define GAP 12
#define FIELD_H 34  // settings field row; six rows plus two group titles fit a column
#define TILE_POINTS 150  // fewer points than tile pixels: LVGL misdraws otherwise

/**********************
 *  STATIC VARIABLES
 **********************/
static GaggiaStateT* state;
static AdvancedSettingsT* advancedSettings;

static lv_obj_t* tabMain;
static lv_obj_t* tabProfile;
static lv_obj_t* tabSettings;
static lv_obj_t* tabAdvance;

static lv_obj_t* selectedProfileLabel;

static lv_obj_t* heat_btn;
static lv_obj_t* heat_btn_label;
static lv_obj_t* boil_btn;
static lv_obj_t* boil_btn_label;
static lv_obj_t* boil_btn_sub;   // pressure and pump on the steam button
static lv_obj_t* btn_names[6];   // name labels, recolored with the checked state
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
static lv_obj_t* time_phase_box;   // top-right of the timer tile: the phases of the running action
static lv_obj_t* time_phase[3];    // BLOOM / WAIT / BREW as a column, or one line (BREW, CLEAN, STEAM, LAST SHOT)
static lv_obj_t* temp_chart;
static lv_obj_t* press_chart;
static lv_chart_series_t* temp_ser;
static lv_chart_series_t* press_ser;
static lv_obj_t* header;
static lv_obj_t* menuDd;

// Field editor: a dim overlay over the top half with the field's name and its value in
// large type, the keyboard in the bottom half. Nothing underneath moves.
static lv_obj_t* editor;
static lv_obj_t* editor_title;
static lv_obj_t* editor_ta;
static lv_obj_t* editor_target;  // the field being edited
static lv_obj_t* editor_kb;      // the keyboard in use (numeric or full)
static lv_obj_t* tabGraph;
static lv_obj_t* tabWifi;
#define VIEW_WIFI 5

// WiFi view
static lv_obj_t* wifiList;
static char wifiNames[10][33];
static lv_obj_t* wifi_ssid_tf;
static lv_obj_t* wifi_pass_tf;
static lv_obj_t* wifi_tz_dd;
static lv_obj_t* wifi_status_label;
static lv_obj_t* wifi_ota_tf;
static bool wifiScanShown = true;  // false while a scan runs and its result is not yet listed
static lv_obj_t* header_clock;
static lv_obj_t* header_wifi;
static lv_obj_t* header_link;  // warning when the controller is silent or ignores commands

// Graph view: 150 s of readings and controller outputs, plus a mode strip.
#define GRAPH_PERIOD_MS 500
#define STRIP_W 776
#define STRIP_H 12
static lv_obj_t* graph_chart;
static lv_chart_series_t* g_temp_ser;
static lv_chart_series_t* g_press_ser;
static lv_chart_series_t* g_boiler_ser;
static lv_chart_series_t* g_pump_ser;
static lv_obj_t* g_legend[4];
static lv_obj_t* g_strip;
static lv_color_t* g_strip_buf;  // allocated through LVGL (PSRAM on the board)


static lv_obj_t* fileList;
static char profileNames[10][PROFILE_NAME_MAX];  // file names behind the list rows
static lv_obj_t* fileName_tf;
static lv_obj_t* fileName_btn;
static lv_obj_t* duplicate_btn;
static lv_obj_t* delete_btn;

static lv_obj_t* edit_notes_tf;
static lv_obj_t* brew_temp_tf;
static lv_obj_t* brew_pressure_tf;
static lv_obj_t* steam_temp_tf;
static lv_obj_t* steam_max_pressure_tf;
static lv_obj_t* steam_shot_tf;
static lv_obj_t* steam_gap_tf;
static lv_obj_t* steam_pump_output_perc_tf;
static lv_obj_t* blooming_pressure_tf;
static lv_obj_t* blooming_fill_time_tf;
static lv_obj_t* blooming_wait_time_tf;
static lv_obj_t* brew_timer_tf;
static lv_obj_t* setBtn;
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

/**********************
 *      UTILS
 **********************/


/**********************
 *      EVENT HANDLING
 **********************/

static void editor_close(bool apply) {
  if (editor_target == NULL) return;
  lv_obj_t* target = editor_target;
  editor_target = NULL;
  if (apply) lv_textarea_set_text(target, lv_textarea_get_text(editor_ta));  // fires VALUE_CHANGED
  if (editor_kb != NULL) {
    lv_keyboard_set_textarea(editor_kb, NULL);
    lv_obj_add_flag(editor_kb, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_add_flag(editor, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_state(editor_ta, LV_STATE_FOCUSED);
  lv_obj_clear_state(target, LV_STATE_FOCUSED);
  // The keyboard acts on the press; without this the release lands on whatever is
  // under the finger once the keyboard is gone (the Cancel button, on the advanced view).
  lv_indev_wait_release(lv_indev_get_act());
  lv_indev_reset(NULL, target);  // so the same field can be tapped again
  lv_event_send(target, apply ? LV_EVENT_READY : LV_EVENT_CANCEL, NULL);
}

static void editor_open(lv_obj_t* field, lv_obj_t* kb) {
  if (editor_target != NULL) return;
  editor_target = field;
  editor_kb = kb;
  const char* title = (const char*)lv_obj_get_user_data(field);
  lv_label_set_text(editor_title, title != NULL ? title : "");
  lv_textarea_set_text(editor_ta, lv_textarea_get_text(field));
  lv_textarea_set_cursor_pos(editor_ta, LV_TEXTAREA_CURSOR_LAST);
  lv_keyboard_set_textarea(kb, editor_ta);
  lv_obj_add_state(editor_ta, LV_STATE_FOCUSED);  // cursor visible from the start
  lv_obj_set_height(kb, LV_VER_RES / 2);
  lv_obj_clear_flag(editor, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(kb);
}

// a tap on the dimmed area (not on the value) closes the editor, keeping the edit
static void editor_bg_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED || editor_target == NULL) return;
  bool changed = strcmp(lv_textarea_get_text(editor_ta), lv_textarea_get_text(editor_target)) != 0;
  editor_close(changed);
}

static void editor_kb_event(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) editor_close(true);
  else if (code == LV_EVENT_CANCEL) editor_close(false);
}

static void editor_create(void) {
  editor = lv_obj_create(lv_layer_top());
  theme_apply_editor(editor);
  lv_obj_set_size(editor, LV_HOR_RES, LV_VER_RES / 2);
  lv_obj_set_pos(editor, 0, 0);
  lv_obj_clear_flag(editor, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(editor, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(editor, editor_bg_clicked, LV_EVENT_CLICKED, NULL);

  editor_title = lv_label_create(editor);
  theme_apply_editor_title(editor_title);
  lv_obj_set_width(editor_title, LV_PCT(90));
  lv_obj_align(editor_title, LV_ALIGN_TOP_MID, 0, 36);

  editor_ta = lv_textarea_create(editor);
  theme_apply_editor_value(editor_ta);
  lv_textarea_set_one_line(editor_ta, true);
  lv_obj_set_width(editor_ta, LV_PCT(90));
  lv_obj_align(editor_ta, LV_ALIGN_TOP_MID, 0, 120);
}

// Tapping a field opens the editor; the editor writes the value back on OK.
static void setting_field_changed(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* ta = lv_event_get_target(e);
  lv_obj_t* kb = lv_event_get_user_data(e);
  if (code == LV_EVENT_FOCUSED) {
    if (lv_indev_get_type(lv_indev_get_act()) != LV_INDEV_TYPE_KEYPAD) editor_open(ta, kb);
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    lv_obj_clear_state(setBtn, LV_STATE_DISABLED);
    lv_obj_clear_state(advancedSetBtn, LV_STATE_DISABLED);
  }
}

static void tab_changed(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    int act = lv_tabview_get_tab_act(tv);
    if (act == 1) updateProfileTab();
    if (act == VIEW_WIFI) wifi_enter();
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
    double newSteamShot = strtod(lv_textarea_get_text(steam_shot_tf), NULL);
    double newSteamGap = strtod(lv_textarea_get_text(steam_gap_tf), NULL);
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
    if (newSteamShot != state->steam_shot_s || newSteamGap != state->steam_gap_s) {
      state->steam_shot_s = newSteamShot;
      state->steam_gap_s = newSteamGap;
      advancedSettings->sendToController = true;  // the timings travel in TUNE
    }
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
    state->cleanLogs = true;  // the sketch starts a new session log
  }
}
static void profile_selected(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    LV_LOG_WARN("profile label Clicked");
    const char* clickedProfileName = (const char*)lv_obj_get_user_data(lv_event_get_target(e));
    if (clickedProfileName == NULL) return;
    if (strlen(clickedProfileName) > 0) {
      lv_textarea_set_text(fileName_tf, clickedProfileName);
      lv_label_set_text(selectedProfileLabel, clickedProfileName);
      writeCurrentProfile(clickedProfileName);
      setupAndReadConfigFile();
      updateProfileTab();
    }
  }
}

void updateProfileTab() {
  char fn[PROFILE_NAME_MAX];
  if (getCurrentProfile(fn, sizeof(fn)) < 0) fn[0] = '\0';

  char names[512];
  if (listProfiles(names, sizeof(names)) < 0) names[0] = '\0';
  int index = 0;
  char* start = names;
  while (*start != '\0' && index < 10) {
    char* end = strchr(start, ';');
    if (end == NULL) break;
    *end = '\0';
    lv_obj_t* child = lv_obj_get_child(fileList, index);
    strncpy(profileNames[index], start, PROFILE_NAME_MAX - 1);
    profileNames[index][PROFILE_NAME_MAX - 1] = '\0';
    char shown[PROFILE_NAME_MAX];
    strcpy(shown, profileNames[index]);
    char* dot = strrchr(shown, '.');
    if (dot != NULL && dot != shown) *dot = '\0';
    lv_label_set_text(child, shown);
    lv_obj_set_user_data(child, profileNames[index]);
    theme_set_selected(child, strcmp(start, fn) == 0);
    lv_obj_clear_flag(child, LV_OBJ_FLAG_HIDDEN);
    start = end + 1;
    index++;
  }
  for (; index < 10; index++) {
    lv_obj_t* child = lv_obj_get_child(fileList, index);
    lv_label_set_text(child, "");
    theme_set_selected(child, false);
    lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
  }
  lv_textarea_set_text(fileName_tf, fn);
  lv_obj_add_flag(fileName_tf, LV_OBJ_FLAG_HIDDEN);
}

// Rename: the first press shows the name field with the keyboard; OK on the
// keyboard (or a second press) applies the new name.
static void fileName_tf_event(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    const char* newProfileName = lv_textarea_get_text(fileName_tf);
    if (strlen(newProfileName) > 0) renameProfile(newProfileName);
    updateProfileTab();
  } else if (code == LV_EVENT_CANCEL) {
    updateProfileTab();
  }
}

static void fileName_btn_clicked(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    if (lv_obj_has_flag(fileName_tf, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(fileName_tf, LV_OBJ_FLAG_HIDDEN);
      lv_event_send(fileName_tf, LV_EVENT_FOCUSED, NULL);
      return;
    }
    const char* newProfileName = lv_textarea_get_text(fileName_tf);
    if (strlen(newProfileName) > 0) renameProfile(newProfileName);
    updateProfileTab();
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
      const char* newProfileName = (const char*)lv_obj_get_user_data(child);
      if (newProfileName != NULL && strlen(newProfileName) > 0) {
        LV_LOG_WARN("new selected profile will be");
        LV_LOG_WARN("newProfileName");
        writeCurrentProfile(newProfileName);
        setupAndReadConfigFile();
      }
      updateProfileTab();
    }
  }
}

// The other views stay reachable while an action runs (the graph is useful then).
void disableTabs() {}
void enableTabs() {}

static void title_clicked(lv_event_t* e) {
  (void)e;
  lv_dropdown_set_selected(menuDd, 0);
  lv_tabview_set_act(tv, 0, LV_ANIM_OFF);
}

static void menu_changed(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    lv_dropdown_close(menuDd);  // close before the view switches, so no half-drawn frame
    lv_tabview_set_act(tv, lv_dropdown_get_selected(menuDd), LV_ANIM_OFF);
  } else if (code == LV_EVENT_READY) {
    // the list exists only while open: make its rows big enough for a finger
    lv_obj_t* list = lv_dropdown_get_list(menuDd);
    lv_obj_t* label = lv_obj_get_child(list, 0);  // LVGL sizes the list from this label right after this event
    lv_obj_set_style_text_font(label, theme_font_value, 0);
    lv_obj_set_style_text_line_space(label, 28, 0);
    lv_obj_set_style_text_font(list, theme_font_value, 0);  // the pressed row is drawn with the list's font
    lv_obj_set_style_text_line_space(list, 28, 0);
    lv_obj_set_style_pad_ver(list, 14, 0);
    lv_obj_set_style_pad_hor(list, 28, 0);
    lv_obj_set_style_bg_color(list, theme_surface(), 0);
    lv_obj_set_style_border_color(list, theme_steel(), 0);
    lv_obj_set_style_max_height(list, LV_VER_RES - 2 * HEADER_H, 0);  // the theme caps lists at 2 x DPI
  }
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

// A full-screen notice above everything (firmware update in progress).
static lv_obj_t* notice;
static lv_obj_t* notice_label;
void ui_show_notice(const char* text) {
  if (notice == NULL) {
    notice = lv_obj_create(lv_layer_top());
    theme_apply_editor(notice);
    lv_obj_set_size(notice, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(notice, 0, 0);
    lv_obj_clear_flag(notice, LV_OBJ_FLAG_SCROLLABLE);
    notice_label = lv_label_create(notice);
    theme_apply_editor_title(notice_label);
    lv_obj_set_width(notice_label, LV_PCT(90));
    lv_obj_center(notice_label);
  }
  lv_label_set_text(notice_label, text);
  lv_obj_clear_flag(notice, LV_OBJ_FLAG_HIDDEN);
}
void ui_hide_notice(void) {
  if (notice != NULL) lv_obj_add_flag(notice, LV_OBJ_FLAG_HIDDEN);
}

// Progress screen: opaque black with one label, so LVGL draws nothing else underneath
// while the firmware is being written.
void ui_show_progress(const char* text) {
  ui_show_notice(text);
  lv_obj_set_style_bg_opa(notice, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(notice, lv_color_black(), 0);
}

// Hooks for the simulator's scenes (checked state of the action buttons).
lv_obj_t* heat_btn_for_scene(void) { return heat_btn; }
lv_obj_t* brew_btn_for_scene(void) { return brew_btn; }
lv_obj_t* steam_btn_for_scene(void) { return boil_btn; }
lv_obj_t* prime_btn_for_scene(void) { return prime_btn; }
lv_obj_t* clean_btn_for_scene(void) { return clean_btn; }
lv_obj_t* auto_btn_for_scene(void) { return auto_btn; }
lv_obj_t* menu_for_scene(void) { return menuDd; }
void show_view_for_scene(int index);
// simulator: edit an advanced field through the editor and report what the field shows
void editadv_for_scene(int step) {
  if (step == 0) {
    updateUI();
    show_view_for_scene(3);
    lv_event_send(boiler_PID_KP_tf, LV_EVENT_FOCUSED, NULL);
    printf("[sim] editor open, target=%p kb=%p field='%s'\n", (void*)editor_target, (void*)editor_kb, lv_textarea_get_text(boiler_PID_KP_tf));
  } else if (step == 1) {
    lv_textarea_set_text(editor_ta, "7");
    lv_event_send(editor_kb, LV_EVENT_READY, NULL);  // what the keyboard's OK does
    printf("[sim] after OK: field='%s' editor_target=%p save disabled=%d\n", lv_textarea_get_text(boiler_PID_KP_tf), (void*)editor_target,
           lv_obj_has_state(advancedSetBtn, LV_STATE_DISABLED));
  } else {
    printf("[sim] later: field='%s' save disabled=%d\n", lv_textarea_get_text(boiler_PID_KP_tf), lv_obj_has_state(advancedSetBtn, LV_STATE_DISABLED));
  }
}
void edit_for_scene(void) {
  updateUI();  // fields are filled from the state on the first refresh
  show_view_for_scene(2);
  lv_event_send(brew_temp_tf, LV_EVENT_FOCUSED, NULL);
}
void dump_tiles_for_scene(void) {
  lv_coord_t* pts = lv_chart_get_y_array(temp_chart, temp_ser);
  int cnt = (int)lv_chart_get_point_count(temp_chart);
  printf("[sim] temp chart points (%d):", cnt);
  for (int i = 0; i < cnt; i++) if (pts[i] != LV_CHART_POINT_NONE) printf(" [%d]=%d", i, (int)pts[i]);
  printf("\n[sim] temp chart x=%d y=%d w=%d h=%d\n", (int)lv_obj_get_x(temp_chart), (int)lv_obj_get_y(temp_chart),
         (int)lv_obj_get_width(temp_chart), (int)lv_obj_get_height(temp_chart));
}
void wifi_for_scene(void) {
  lv_tabview_set_act(tv, VIEW_WIFI, LV_ANIM_OFF);
}
void show_view_for_scene(int index) {
  lv_dropdown_set_selected(menuDd, index);
  lv_tabview_set_act(tv, index, LV_ANIM_OFF);
}

// Profile names are file names on the card; show them without the extension.
static void set_profile_title(const char* name) {
  char shown[PROFILE_NAME_MAX];
  strncpy(shown, name, sizeof(shown) - 1);
  shown[sizeof(shown) - 1] = '\0';
  char* dot = strrchr(shown, '.');
  if (dot != NULL && dot != shown) *dot = '\0';
  lv_label_set_text(selectedProfileLabel, shown);
  // the notes follow the title; the menu button keeps the last 80 px
  lv_obj_update_layout(selectedProfileLabel);
  lv_coord_t titleEnd = lv_obj_get_x2(selectedProfileLabel) + 20;
  lv_obj_set_width(main_notes_label, LV_HOR_RES - titleEnd - 190);
  lv_obj_align_to(main_notes_label, selectedProfileLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 2);
}

// One line of the timer tile's phase column: `color` at full strength when active, dim otherwise.
static void phase_line(int i, const char* text, lv_color_t color, bool active) {
  lv_label_set_text(time_phase[i], text);
  lv_obj_set_style_text_color(time_phase[i], color, 0);
  lv_obj_set_style_text_opa(time_phase[i], active ? LV_OPA_COVER : LV_OPA_70, 0);
  lv_obj_clear_flag(time_phase[i], LV_OBJ_FLAG_HIDDEN);
}

void updateUI() {
  static uint8_t tick = 0;
  LV_LOG_TRACE("updating UI");
  if (state->hasConfigChanged) {
    lv_label_set_text_fmt(heat_btn_label, "%.0f °C", state->boilerSetPoint);
    lv_label_set_text_fmt(boil_btn_label, "%.0f °C", state->steamSetPoint);
    lv_label_set_text_fmt(boil_btn_sub, "%.0f bar • %.0f%%", state->steam_max_pressure, state->steam_pump_output_percent);
    lv_label_set_text_fmt(brew_btn_label, "%.1f bar", state->pressureSetPoint);
    lv_label_set_text_fmt(prime_btn_label, "%.0f + %.0f s", state->blooming_fill_time, state->blooming_wait_time);
    lv_label_set_text_fmt(auto_btn_label, "%.0f s", state->brew_timer);
    set_profile_title(state->profile_name);
  }

  // readings and their curves (5 Hz here; temperature once a second)
  lv_label_set_text_fmt(temp_label, "%.0f", state->tempRead);
  lv_label_set_text_fmt(press_label, "%.1f", state->pressureRead);
  history_push(HISTORY_PRESSURE, state->pressureRead);
  lv_chart_set_next_value(press_chart, press_ser, (lv_coord_t)(state->pressureRead * 10));
  if (++tick >= 5) {
    tick = 0;
    history_push(HISTORY_TEMPERATURE, state->tempRead);
    lv_chart_set_next_value(temp_chart, temp_ser, (lv_coord_t)(state->tempRead * 10));
  }
  bool running = state->actionStartTime > 0 && state->actionStopTime == 0;
  theme_set_hot(temp_label, temp_chart, temp_ser, state->isBoilerOn || state->isSteaming,
                state->isSteaming ? theme_steam() : theme_amber());
  theme_set_hot(press_label, press_chart, press_ser,
                state->isSolenoidOn || state->isBrewing || state->isCleaning || state->isBlooming || state->isAuto,
                theme_pressure());
  theme_set_hot(time_label, NULL, NULL, running, theme_pump());

  // Bloom and auto end on their own (sequencer): release the buttons and the menu
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
      lv_label_set_text_fmt(time_label, "%d", (state->actionStopTime - state->actionStartTime) / 1000);
    } else {
      lv_label_set_text_fmt(time_label, "%d", (millis() - state->actionStartTime) / 1000);
    }
  } else {
    lv_label_set_text(time_label, "0");
  }
  // Phases of the running action. Prime and auto list every phase they will go
  // through, the current one lit and the others dim; a single-phase action shows its
  // name; idle shows the last shot.
  {
    int ph = sequencerPhase();
    int n = 0;
    if (state->isBlooming || state->isAuto) {
      if (sequencerBloomConfigured(state)) {
        phase_line(n++, "BLOOM", theme_pressure(), ph == PHASE_BLOOM_FILL);
        phase_line(n++, "WAIT", theme_amber(), ph == PHASE_BLOOM_WAIT);
      }
      if (state->isAuto) phase_line(n++, "BREW", theme_pump(), ph == PHASE_BREW);
    } else if (state->isBrewing) {
      phase_line(n++, "BREW", theme_pump(), true);
    } else if (state->isCleaning) {
      phase_line(n++, "CLEAN", theme_clean(), true);
    } else if (state->isSteaming) {
      phase_line(n++, "STEAM", theme_steam(), true);
    } else if (state->lastBrewTime > 0) {
      char t[24];
      snprintf(t, sizeof(t), "LAST SHOT %.0f S", state->lastBrewTime);
      phase_line(n++, t, theme_muted(), true);
    }
    for (int i = n; i < 3; i++) lv_obj_add_flag(time_phase[i], LV_OBJ_FLAG_HIDDEN);
  }

  graph_update(millis());
  {
    static uint32_t lastNet = 0;
    uint32_t nowMs = millis();
    if (nowMs - lastNet >= 1000) {
      lastNet = nowMs;
      header_net_refresh();
      if (lv_tabview_get_tab_act(tv) == VIEW_WIFI) wifi_refresh();
    }
  }

  // button labels follow the checked state (amber names when idle, ink on a lit button)
  {
    lv_obj_t* btns[6] = { heat_btn, brew_btn, prime_btn, boil_btn, clean_btn, auto_btn };
    lv_obj_t* values[6] = { heat_btn_label, brew_btn_label, prime_btn_label, boil_btn_label, clean_btn_label, auto_btn_label };
    lv_color_t idle[6] = { theme_amber(), theme_pressure(), theme_pressure(), theme_steam(), theme_clean(), theme_pump() };
    for (int i = 0; i < 6; i++) {
      theme_set_btn_labels(btn_names[i], values[i], btns[i] == boil_btn ? boil_btn_sub : NULL,
                           lv_obj_has_state(btns[i], LV_STATE_CHECKED), idle[i]);
    }
  }

  // Link warning in the header: the controller is silent, or it answers but reports
  // the link down (it is not acting on the buttons). The notes make room for it.
  {
    const char* warning = NULL;
    if (!state->ctrlAlive) warning = "CONTROLLER SILENT";
    else if (!state->linkOk) warning = "CONTROLLER IGNORES COMMANDS";
    else if (state->pressStale) warning = "NO PRESSURE READING";
    if (warning != NULL) {
      lv_label_set_text(header_link, warning);
      lv_obj_clear_flag(header_link, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(main_notes_label, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(header_link, LV_OBJ_FLAG_HIDDEN);
      if (state->notes[0] == '\0') lv_obj_add_flag(main_notes_label, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_clear_flag(main_notes_label, LV_OBJ_FLAG_HIDDEN);
    }
  }

  updateSettings();
}

void updateSettings() {
  LV_LOG_TRACE("updating settings fields");
  bool configRefreshed = false, advancedRefreshed = false;

  if (state->hasConfigChanged) {
    // LV_LOG_WARN("updating config fields");

    char t[100];
    lv_label_set_text(main_notes_label, state->notes);
    lv_textarea_set_text(edit_notes_tf, state->notes);

    sprintf(t, "%g", state->boilerSetPoint);
    lv_textarea_set_text(brew_temp_tf, t);

    sprintf(t, "%g", state->pressureSetPoint);
    lv_textarea_set_text(brew_pressure_tf, t);

    sprintf(t, "%g", state->steamSetPoint);
    lv_textarea_set_text(steam_temp_tf, t);

    sprintf(t, "%g", state->steam_max_pressure);
    lv_textarea_set_text(steam_max_pressure_tf, t);

    sprintf(t, "%g", state->steam_pump_output_percent);
    lv_textarea_set_text(steam_pump_output_perc_tf, t);

    sprintf(t, "%g", state->steam_shot_s);
    lv_textarea_set_text(steam_shot_tf, t);

    sprintf(t, "%g", state->steam_gap_s);
    lv_textarea_set_text(steam_gap_tf, t);

    sprintf(t, "%g", state->blooming_pressure);
    lv_textarea_set_text(blooming_pressure_tf, t);

    sprintf(t, "%g", state->blooming_fill_time);
    lv_textarea_set_text(blooming_fill_time_tf, t);

    sprintf(t, "%g", state->blooming_wait_time);
    lv_textarea_set_text(blooming_wait_time_tf, t);

    sprintf(t, "%g", state->brew_timer);
    lv_textarea_set_text(brew_timer_tf, t);

    state->hasConfigChanged = false;
    configRefreshed = true;
  }
  if (advancedSettings->userChanged) {
    // LV_LOG_WARN("updating advanced Settings fields");

    char t[100];
    sprintf(t, "%g", advancedSettings->boiler_bb_range);
    lv_textarea_set_text(boiler_bb_range_tf, t);
    sprintf(t, "%g", advancedSettings->boiler_PID_cycle);
    lv_textarea_set_text(boiler_PID_cycle_tf, t);
    sprintf(t, "%g", advancedSettings->boiler_PID_KP);
    lv_textarea_set_text(boiler_PID_KP_tf, t);
    sprintf(t, "%g", advancedSettings->boiler_PID_KI);
    lv_textarea_set_text(boiler_PID_KI_tf, t);
    sprintf(t, "%g", advancedSettings->boiler_PID_KD);
    lv_textarea_set_text(boiler_PID_KD_tf, t);
    sprintf(t, "%g", advancedSettings->pump_max_step_up);
    lv_textarea_set_text(pump_max_step_up_tf, t);
    sprintf(t, "%g", advancedSettings->pump_KP);
    lv_textarea_set_text(pump_KP_tf, t);
    sprintf(t, "%g", advancedSettings->pump_KI);
    lv_textarea_set_text(pump_KI_tf, t);
    sprintf(t, "%g", advancedSettings->pump_KD);
    lv_textarea_set_text(pump_KD_tf, t);
    sprintf(t, "%g", advancedSettings->unused1);
    lv_textarea_set_text(unused1_tf, t);

    advancedSettings->userChanged = false;
    advancedRefreshed = true;
  }
  // setting a text area emits a change event, which enables the Save buttons: undo that
  if (configRefreshed) lv_obj_add_state(setBtn, LV_STATE_DISABLED);
  if (advancedRefreshed) lv_obj_add_state(advancedSetBtn, LV_STATE_DISABLED);
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

  theme_init();
  font_large = theme_font_value;
  font_normal = theme_font_name;
  theme_apply_screen(lv_scr_act());
  lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);

  // Header: profile name, notes, menu. The views below live in a tabview whose own
  // tab bar is collapsed; the menu switches between them.
  header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, LV_HOR_RES, HEADER_H);
  lv_obj_set_pos(header, 0, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(header, 0, 0);
  lv_obj_set_style_pad_left(header, 16, 0);
  lv_obj_set_style_pad_right(header, 12, 0);
  theme_apply_header(header);

  selectedProfileLabel = lv_label_create(header);
  theme_apply_title(selectedProfileLabel);
  lv_obj_align(selectedProfileLabel, LV_ALIGN_LEFT_MID, 0, 0);

  main_notes_label = lv_label_create(header);
  theme_apply_header_notes(main_notes_label);
  lv_label_set_long_mode(main_notes_label, LV_LABEL_LONG_DOT);
  lv_obj_set_width(main_notes_label, 300);
  lv_obj_align(main_notes_label, LV_ALIGN_LEFT_MID, 200, 1);  // re-placed after the title in set_profile_title()

  header_wifi = lv_label_create(header);
  theme_apply_header_notes(header_wifi);
  lv_label_set_text(header_wifi, LV_SYMBOL_WIFI);
  lv_obj_align(header_wifi, LV_ALIGN_RIGHT_MID, -72, 0);

  header_clock = lv_label_create(header);
  theme_apply_header_notes(header_clock);
  lv_label_set_text(header_clock, "");
  lv_obj_align(header_clock, LV_ALIGN_RIGHT_MID, -108, 1);

  header_link = lv_label_create(header);
  theme_apply_header_notes(header_link);
  lv_obj_set_style_text_color(header_link, theme_steam(), 0);
  lv_label_set_text(header_link, "");
  lv_obj_align(header_link, LV_ALIGN_RIGHT_MID, -170, 1);
  lv_obj_add_flag(header_link, LV_OBJ_FLAG_HIDDEN);

  menuDd = lv_dropdown_create(header);
  lv_dropdown_set_options_static(menuDd, "Main\nProfiles\nSettings\nAdvanced\nGraph");
  lv_dropdown_set_text(menuDd, LV_SYMBOL_LIST);
  lv_dropdown_set_symbol(menuDd, NULL);
  lv_dropdown_set_selected_highlight(menuDd, false);
  lv_dropdown_set_dir(menuDd, LV_DIR_LEFT);  // the button sits at the screen edge; the list opens leftwards
  lv_obj_set_size(menuDd, 56, 34);
  lv_obj_align(menuDd, LV_ALIGN_RIGHT_MID, 0, 0);
  theme_apply_menu(menuDd);
  lv_obj_add_event_cb(menuDd, menu_changed, LV_EVENT_ALL, NULL);

  tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 0);
  lv_obj_set_size(tv, LV_HOR_RES, LV_VER_RES - HEADER_H);
  lv_obj_set_pos(tv, 0, HEADER_H);
  lv_obj_add_event_cb(tv, tab_changed, LV_EVENT_ALL, NULL);

  tabMain = lv_tabview_add_tab(tv, "Main");
  tabProfile = lv_tabview_add_tab(tv, "Profiles");
  tabSettings = lv_tabview_add_tab(tv, "Settings");
  tabAdvance = lv_tabview_add_tab(tv, "Advanced");
  tabGraph = lv_tabview_add_tab(tv, "Graph");
  tabWifi = lv_tabview_add_tab(tv, "WiFi");  // not in the menu: reached from Advanced
  // the tabview and its pages come with the default theme's dark grey: make them black
  theme_apply_screen(tv);
  theme_apply_screen(lv_tabview_get_content(tv));
  lv_obj_t* pages[] = { tabMain, tabProfile, tabSettings, tabAdvance, tabGraph, tabWifi };
  for (int i = 0; i < 6; i++) theme_apply_screen(pages[i]);

  main_create(tabMain);
  profile_create(tabProfile);
  settings_create(tabSettings);
  advancedSettings_create(tabAdvance);
  graph_create(tabGraph);
  wifi_create(tabWifi);
  editor_create();

  // shortcut: the profile name (and the notes) bring you back to the main screen
  lv_obj_add_flag(selectedProfileLabel, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(main_notes_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(selectedProfileLabel, title_clicked, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(main_notes_label, title_clicked, LV_EVENT_CLICKED, NULL);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

// One reading tile: curve behind, label top-left, big value + unit bottom-left.
static lv_obj_t* tile_create(lv_obj_t* parent, const char* name, const char* unit, lv_color_t color, lv_obj_t** value_out,
                             lv_obj_t** chart_out, lv_chart_series_t** ser_out, lv_coord_t range_min, lv_coord_t range_max) {
  lv_obj_t* tile = lv_obj_create(parent);
  theme_apply_tile(tile);
  lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

  if (chart_out != NULL) {
    lv_obj_t* chart = lv_chart_create(tile);
    theme_apply_chart(chart);
    lv_obj_set_size(chart, LV_PCT(100), LV_PCT(100));
    lv_obj_center(chart);
    lv_chart_set_point_count(chart, TILE_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, range_min, range_max);
    *chart_out = chart;
    *ser_out = theme_chart_series(chart);
    lv_chart_set_all_value(chart, *ser_out, LV_CHART_POINT_NONE);  // no line until real readings arrive
  }

  lv_obj_t* label = lv_label_create(tile);
  theme_apply_tile_label(label);
  lv_obj_set_style_text_color(label, color, 0);
  lv_label_set_text(label, name);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 14, 10);

  lv_obj_t* row = lv_obj_create(tile);  // value + unit, bottom-left, reflows when the value width changes
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  lv_obj_set_style_pad_column(row, 6, 0);
  lv_obj_align(row, LV_ALIGN_BOTTOM_LEFT, 14, -6);

  lv_obj_t* value = lv_label_create(row);
  theme_apply_tile_value(value);
  lv_label_set_text(value, "--");
  *value_out = value;

  lv_obj_t* unitLabel = lv_label_create(row);
  theme_apply_tile_unit(unitLabel);
  lv_label_set_text(unitLabel, unit);
  lv_obj_set_style_pad_bottom(unitLabel, 6, 0);
  return tile;
}

// One action button: name over value.
static lv_obj_t* action_btn_create(lv_obj_t* parent, const char* name, lv_color_t color, lv_obj_t** value_out, lv_obj_t** sub_out) {
  lv_obj_t* btn = lv_btn_create(parent);
  theme_apply_btn(btn, color);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_add_event_cb(btn, main_btn_clicked, LV_EVENT_ALL, NULL);
  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(btn, 8, 0);

  lv_obj_t* nameLabel = lv_label_create(btn);
  theme_apply_btn_name(nameLabel);
  lv_label_set_text(nameLabel, name);
  for (int i = 0; i < 6; i++) if (btn_names[i] == NULL) { btn_names[i] = nameLabel; break; }

  lv_obj_t* value = lv_label_create(btn);
  theme_apply_btn_value(value);
  lv_label_set_text(value, "");
  *value_out = value;

  if (sub_out != NULL) {
    lv_obj_t* sub = lv_label_create(btn);
    theme_apply_btn_sub(sub);
    lv_label_set_text(sub, "");
    *sub_out = sub;
  }
  return btn;
}

static void main_create(lv_obj_t* parent) {
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(parent, GAP, 0);
  lv_obj_set_style_pad_row(parent, GAP, 0);
  lv_obj_set_style_pad_column(parent, GAP, 0);

  static lv_coord_t col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t row_dsc[] = { TILE_H, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);

  lv_obj_t* temp_tile = tile_create(parent, "BOILER", "°C", theme_amber(), &temp_label, &temp_chart, &temp_ser, 20 * 10, 160 * 10);
  lv_obj_t* press_tile = tile_create(parent, "PRESSURE", "bar", theme_pressure(), &press_label, &press_chart, &press_ser, 0, 14 * 10);
  lv_obj_t* time_tile = tile_create(parent, "TIMER", "s", theme_pump(), &time_label, NULL, NULL, 0, 0);
  time_phase_box = lv_obj_create(time_tile);  // phase names, left-aligned in a column at the top right
  lv_obj_remove_style_all(time_phase_box);
  lv_obj_set_size(time_phase_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(time_phase_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(time_phase_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(time_phase_box, 6, 0);
  lv_obj_align(time_phase_box, LV_ALIGN_TOP_RIGHT, -14, 10);
  for (int i = 0; i < 3; i++) {
    time_phase[i] = lv_label_create(time_phase_box);
    theme_apply_tile_label(time_phase[i]);
    lv_label_set_text(time_phase[i], "");
    lv_obj_add_flag(time_phase[i], LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_set_grid_cell(temp_tile, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(press_tile, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  lv_obj_set_grid_cell(time_tile, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  heat_btn = action_btn_create(parent, "Heat", theme_amber(), &heat_btn_label, NULL);
  brew_btn = action_btn_create(parent, "Brew", theme_pressure(), &brew_btn_label, NULL);
  prime_btn = action_btn_create(parent, "Prime", theme_pressure(), &prime_btn_label, NULL);
  boil_btn = action_btn_create(parent, "Steam", theme_steam(), &boil_btn_label, &boil_btn_sub);
  lv_obj_set_style_pad_row(boil_btn, 2, 0);  // three lines have to fit
  clean_btn = action_btn_create(parent, "Clean", theme_clean(), &clean_btn_label, NULL);
  auto_btn = action_btn_create(parent, "Auto", theme_pump(), &auto_btn_label, NULL);
  lv_label_set_text(clean_btn_label, "9 bar");

  lv_obj_set_grid_cell(heat_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(brew_btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(prime_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_set_grid_cell(boil_btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(clean_btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_set_grid_cell(auto_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
}

// ---------------------------------------------------------------- WiFi view

static void wifi_btn_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) lv_tabview_set_act(tv, VIEW_WIFI, LV_ANIM_OFF);
}

static void wifi_row_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  const char* name = (const char*)lv_obj_get_user_data(lv_event_get_target(e));
  if (name != NULL && name[0] != '\0') lv_textarea_set_text(wifi_ssid_tf, name);
}

static void wifi_scan_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if (netScanStart() == 0) {
    wifiScanShown = false;
    lv_label_set_text(wifi_status_label, "Scanning...");
  }
}

static void wifi_connect_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  const char* ssid = lv_textarea_get_text(wifi_ssid_tf);
  if (ssid[0] == '\0') return;
  netSetCredentials(ssid, lv_textarea_get_text(wifi_pass_tf));
  wifi_refresh();
}

static void wifi_forget_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  netForget();
  lv_textarea_set_text(wifi_ssid_tf, "");
  lv_textarea_set_text(wifi_pass_tf, "");
  wifi_refresh();
}

static void wifi_ota_changed(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_READY) {  // the editor applied a new value
    const char* pw = lv_textarea_get_text(wifi_ota_tf);
    if (pw[0] != '\0') netSetOtaPassword(pw);
  }
}

static void wifi_tz_changed(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) netSetTimezone((int)lv_dropdown_get_selected(wifi_tz_dd));
}

static void wifi_create(lv_obj_t* parent) {
  theme_apply_view(parent);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  static lv_coord_t col_dsc[] = { LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t row_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);
  lv_obj_set_style_pad_column(parent, 24, 0);

  // left: the networks found
  wifiList = lv_obj_create(parent);
  theme_apply_list(wifiList);
  lv_obj_set_flex_flow(wifiList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_grid_cell(wifiList, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  for (int i = 0; i < 10; i++) {
    lv_obj_t* lbl = lv_label_create(wifiList);
    theme_apply_list_row(lbl);
    lv_obj_set_style_text_font(lbl, theme_font_name, 0);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(lbl, wifi_row_clicked, LV_EVENT_ALL, NULL);
    lv_label_set_text(lbl, "");
  }

  // right: network, password, time zone, actions, status
  lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(kb, editor_kb_event, LV_EVENT_ALL, NULL);

  lv_obj_t* right = column_create(parent, 1, 0);
  lv_obj_t* g = group_create(right, "NETWORK");
  wifi_ssid_tf = field_create(g, "Name", kb);
  lv_obj_set_user_data(wifi_ssid_tf, (void*)"NETWORK NAME");
  lv_obj_set_width(wifi_ssid_tf, 220);
  wifi_pass_tf = field_create(g, "Password", kb);
  lv_obj_set_user_data(wifi_pass_tf, (void*)"PASSWORD");
  lv_obj_set_width(wifi_pass_tf, 220);

  g = group_create(right, "TIME ZONE");
  wifi_tz_dd = lv_dropdown_create(g);
  lv_dropdown_set_options_static(wifi_tz_dd, timezoneOptions());
  theme_apply_field(wifi_tz_dd);
  lv_obj_set_size(wifi_tz_dd, LV_PCT(100), 40);
  lv_obj_add_event_cb(wifi_tz_dd, wifi_tz_changed, LV_EVENT_ALL, NULL);

  g = group_create(right, "FIRMWARE UPDATE");
  wifi_ota_tf = field_create(g, "Password", kb);
  lv_obj_set_user_data(wifi_ota_tf, (void*)"UPDATE PASSWORD");
  lv_obj_set_width(wifi_ota_tf, 220);
  lv_obj_add_event_cb(wifi_ota_tf, wifi_ota_changed, LV_EVENT_ALL, NULL);

  lv_obj_t* row = lv_obj_create(right);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), 52);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(row, GAP, 0);
  view_btn_create(row, "Scan", wifi_scan_clicked);
  view_btn_create(row, "Connect", wifi_connect_clicked);
  view_btn_create(row, "Forget", wifi_forget_clicked);

  wifi_status_label = lv_label_create(right);
  theme_apply_muted(wifi_status_label);
  lv_obj_set_style_text_font(wifi_status_label, theme_font_name, 0);
  lv_obj_set_width(wifi_status_label, LV_PCT(100));
  lv_label_set_long_mode(wifi_status_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(wifi_status_label, "");
}

// Called when the view opens: show the saved network, start a scan.
static void wifi_enter(void) {
  struct NetStatus st;
  netGetStatus(&st);
  lv_textarea_set_text(wifi_ssid_tf, st.ssid);
  lv_textarea_set_text(wifi_ota_tf, netOtaPassword());
  lv_dropdown_set_selected(wifi_tz_dd, (uint16_t)netTimezone());
  if (netScanStart() == 0) wifiScanShown = false;
  wifi_refresh();
}

// Status line and scan list; called once a second while the view is shown.
static void wifi_refresh(void) {
  struct NetStatus st;
  netGetStatus(&st);
  char clock[16];
  netLocalTime(clock, sizeof(clock), "%H:%M");
  switch (st.state) {
    case NET_CONNECTED: lv_label_set_text_fmt(wifi_status_label, "Connected to %s\n%s  %d dBm  %s", st.ssid, st.ip, st.rssi, clock); break;
    case NET_CONNECTING: lv_label_set_text_fmt(wifi_status_label, "Connecting to %s...", st.ssid); break;
    case NET_FAILED: lv_label_set_text_fmt(wifi_status_label, "Could not join %s, retrying", st.ssid); break;
    default: lv_label_set_text(wifi_status_label, "No network saved"); break;
  }
  if (!wifiScanShown) {
    int n = netScanCount();
    if (n >= 0) {
      wifiScanShown = true;
      for (int i = 0; i < 10; i++) {
        lv_obj_t* row = lv_obj_get_child(wifiList, i);
        if (i < n) {
          strncpy(wifiNames[i], netScanSsid(i), 32);
          wifiNames[i][32] = '\0';
          lv_label_set_text_fmt(row, "%s   %d", wifiNames[i], netScanRssi(i));
          lv_obj_set_user_data(row, wifiNames[i]);
          theme_set_selected(row, strcmp(wifiNames[i], st.ssid) == 0);
          lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
        } else {
          lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
        }
      }
      if (n == 0) lv_label_set_text(wifi_status_label, "No network found");
    }
  }
}

// Header clock and WiFi symbol: once a second, whatever the view.
static void header_net_refresh(void) {
  struct NetStatus st;
  netGetStatus(&st);
  lv_obj_set_style_text_color(header_wifi, st.state == NET_CONNECTED ? theme_text() : lv_color_hex(0x3A4048), 0);
  char clock[16];
  if (st.timeValid && netLocalTime(clock, sizeof(clock), "%H:%M")) lv_label_set_text(header_clock, clock);
  else lv_label_set_text(header_clock, "");
}

static void graph_create(lv_obj_t* parent) {
  theme_apply_view(parent);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(parent, 8, 0);

  lv_obj_t* legend = lv_obj_create(parent);
  lv_obj_remove_style_all(legend);
  lv_obj_set_size(legend, LV_PCT(100), 30);
  lv_obj_set_flex_flow(legend, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(legend, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_color_t colors[4] = { theme_amber(), theme_pressure(), theme_heater(), theme_pump() };
  for (int i = 0; i < 4; i++) {
    g_legend[i] = lv_label_create(legend);
    theme_apply_legend(g_legend[i], colors[i]);
    lv_label_set_text(g_legend[i], "");
  }

  graph_chart = lv_chart_create(parent);
  theme_apply_graph(graph_chart);
  lv_obj_set_width(graph_chart, LV_PCT(100));
  lv_obj_set_flex_grow(graph_chart, 1);
  lv_chart_set_point_count(graph_chart, HISTORY_CAPACITY);
  lv_chart_set_update_mode(graph_chart, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_range(graph_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 160);    // °C; outputs are scaled so 100 % is the top
  lv_chart_set_range(graph_chart, LV_CHART_AXIS_SECONDARY_Y, 0, 140);  // bar x10
  g_boiler_ser = lv_chart_add_series(graph_chart, theme_heater(), LV_CHART_AXIS_PRIMARY_Y);
  g_pump_ser = lv_chart_add_series(graph_chart, theme_pump(), LV_CHART_AXIS_PRIMARY_Y);
  g_temp_ser = lv_chart_add_series(graph_chart, theme_amber(), LV_CHART_AXIS_PRIMARY_Y);
  g_press_ser = lv_chart_add_series(graph_chart, theme_pressure(), LV_CHART_AXIS_SECONDARY_Y);
  lv_chart_set_all_value(graph_chart, g_boiler_ser, LV_CHART_POINT_NONE);
  lv_chart_set_all_value(graph_chart, g_pump_ser, LV_CHART_POINT_NONE);
  lv_chart_set_all_value(graph_chart, g_temp_ser, LV_CHART_POINT_NONE);
  lv_chart_set_all_value(graph_chart, g_press_ser, LV_CHART_POINT_NONE);

  g_strip = lv_canvas_create(parent);
  g_strip_buf = (lv_color_t*)lv_mem_alloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(STRIP_W, STRIP_H));
  lv_canvas_set_buffer(g_strip, g_strip_buf, STRIP_W, STRIP_H, LV_IMG_CF_TRUE_COLOR);
  lv_canvas_fill_bg(g_strip, lv_color_black(), LV_OPA_COVER);
  lv_obj_set_size(g_strip, STRIP_W, STRIP_H);
}

static lv_color_t mode_color(int mode) {
  switch (mode) {
    case 1: return theme_pressure();
    case 2: return theme_steam();
    case 3: return theme_clean();
    default: return lv_color_black();
  }
}

static void graph_update(uint32_t now) {
  static uint32_t last = 0;
  if (now - last < GRAPH_PERIOD_MS) return;
  last = now;

  history_push(HISTORY_G_TEMP, state->tempRead);
  history_push(HISTORY_G_PRESS, state->pressureRead);
  history_push(HISTORY_G_BOILER, state->boilerOut);
  history_push(HISTORY_G_PUMP, state->pumpOut * 100.0f / 127.0f);
  history_push(HISTORY_G_MODE, (float)state->ctrlMode);
  lv_chart_set_next_value(graph_chart, g_temp_ser, (lv_coord_t)state->tempRead);
  lv_chart_set_next_value(graph_chart, g_press_ser, (lv_coord_t)(state->pressureRead * 10));
  lv_chart_set_next_value(graph_chart, g_boiler_ser, (lv_coord_t)(state->boilerOut * 1.6f));
  lv_chart_set_next_value(graph_chart, g_pump_ser, (lv_coord_t)(state->pumpOut * 100.0f / 127.0f * 1.6f));

  if (lv_tabview_get_tab_act(tv) != 4) return;  // the rest is only drawn while the view is shown

  lv_label_set_text_fmt(g_legend[0], "BOILER %.0f °C", state->tempRead);
  lv_label_set_text_fmt(g_legend[1], "PRESSURE %.1f bar", state->pressureRead);
  lv_label_set_text_fmt(g_legend[2], "HEATER %.0f%%", state->boilerOut);
  lv_label_set_text_fmt(g_legend[3], "PUMP %.0f%%", state->pumpOut * 100.0f / 127.0f);

  // mode strip: one column per sample, right-aligned like the chart
  lv_canvas_fill_bg(g_strip, lv_color_black(), LV_OPA_COVER);
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  int n = history_count(HISTORY_G_MODE);
  for (int i = 0; i < n; i++) {
    int mode = (int)history_at(HISTORY_G_MODE, i);
    if (mode == 0) continue;
    int slot = HISTORY_CAPACITY - n + i;
    lv_coord_t x0 = (lv_coord_t)((long)slot * STRIP_W / HISTORY_CAPACITY);
    lv_coord_t x1 = (lv_coord_t)((long)(slot + 1) * STRIP_W / HISTORY_CAPACITY);
    dsc.bg_color = mode_color(mode);
    lv_canvas_draw_rect(g_strip, x0, 0, x1 - x0, STRIP_H, &dsc);
  }
}

static void profile_create(lv_obj_t* parent) {
  theme_apply_view(parent);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  static lv_coord_t col_dsc[] = { LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t row_dsc[] = { LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);

  // the profile list: one row per file, the selected one highlighted
  fileList = lv_obj_create(parent);
  theme_apply_list(fileList);
  lv_obj_set_flex_flow(fileList, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_grid_cell(fileList, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  for (int i = 0; i < 10; i++) {
    lv_obj_t* lbl = lv_label_create(fileList);
    theme_apply_list_row(lbl);
    lv_obj_set_width(lbl, LV_PCT(100));
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(lbl, profile_selected, LV_EVENT_ALL, NULL);
    lv_label_set_text(lbl, "");
  }

  // the actions: name field (shown by Rename) and three tall buttons
  lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(kb, editor_kb_event, LV_EVENT_ALL, NULL);

  lv_obj_t* actions = lv_obj_create(parent);
  lv_obj_remove_style_all(actions);
  lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(actions, GAP, 0);
  lv_obj_set_grid_cell(actions, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  fileName_tf = lv_textarea_create(actions);
  theme_apply_field(fileName_tf);
  lv_obj_set_user_data(fileName_tf, (void*)"PROFILE NAME");
  lv_obj_set_width(fileName_tf, LV_PCT(100));
  lv_textarea_set_max_length(fileName_tf, PROFILE_NAME_MAX - 1);
  lv_textarea_set_one_line(fileName_tf, true);
  lv_obj_add_flag(fileName_tf, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(fileName_tf, setting_field_changed, LV_EVENT_ALL, kb);
  lv_obj_add_event_cb(fileName_tf, fileName_tf_event, LV_EVENT_ALL, NULL);

  const char* names[] = { "Rename", "Duplicate", "Delete" };
  lv_event_cb_t cbs[] = { fileName_btn_clicked, duplicate_btn_clicked, delete_btn_clicked };
  lv_obj_t** btns[] = { &fileName_btn, &duplicate_btn, &delete_btn };
  for (int i = 0; i < 3; i++) {
    lv_obj_t* btn = lv_btn_create(actions);
    theme_apply_btn(btn, theme_amber());
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, cbs[i], LV_EVENT_ALL, NULL);
    lv_obj_t* lbl = lv_label_create(btn);
    theme_apply_btn_name(lbl);
    lv_label_set_text(lbl, names[i]);
    lv_obj_center(lbl);
    *btns[i] = btn;
  }
}

// A titled group of fields (flex column) inside a settings column.
static lv_obj_t* group_create(lv_obj_t* column, const char* title) {
  lv_obj_t* group = lv_obj_create(column);
  lv_obj_remove_style_all(group);
  lv_obj_set_width(group, LV_PCT(100));
  lv_obj_set_height(group, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(group, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(group, 4, 0);
  lv_obj_t* label = lv_label_create(group);
  theme_apply_group_title(label);
  lv_label_set_text(label, title);
  return group;
}

// One "label ........ [value]" row in a group.
static lv_obj_t* field_create(lv_obj_t* group, const char* text, lv_obj_t* kb) {
  lv_obj_t* row = lv_obj_create(group);
  lv_obj_remove_style_all(row);
  lv_obj_set_width(row, LV_PCT(100));
  lv_obj_set_height(row, FIELD_H);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* label = lv_label_create(row);
  theme_apply_field_label(label);
  lv_label_set_text(label, text);
  lv_obj_set_flex_grow(label, 1);

  lv_obj_t* tf = lv_textarea_create(row);
  theme_apply_field(tf);
  lv_obj_set_user_data(tf, (void*)text);  // the editor shows it as the title
  lv_textarea_set_one_line(tf, true);
  lv_obj_set_size(tf, 150, FIELD_H);
  lv_obj_set_scrollbar_mode(tf, LV_SCROLLBAR_MODE_OFF);  // one line, no need for the bar the shorter box would show
  lv_obj_add_event_cb(tf, setting_field_changed, LV_EVENT_ALL, kb);
  return tf;
}

// A settings column: groups stacked top to bottom.
static lv_obj_t* column_create(lv_obj_t* parent, int col, int row) {
  lv_obj_t* column = lv_obj_create(parent);
  lv_obj_remove_style_all(column);
  lv_obj_set_height(column, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(column, 12, 0);
  lv_obj_set_grid_cell(column, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_START, row, 1);
  return column;
}

static lv_obj_t* view_btn_create(lv_obj_t* parent, const char* text, lv_event_cb_t cb) {
  lv_obj_t* btn = lv_btn_create(parent);
  theme_apply_btn(btn, theme_amber());
  lv_obj_set_height(btn, 52);
  lv_obj_set_flex_grow(btn, 1);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, NULL);
  lv_obj_t* lbl = lv_label_create(btn);
  theme_apply_btn_name(lbl);
  lv_label_set_text(lbl, text);
  lv_obj_center(lbl);
  return btn;
}

static lv_obj_t* numeric_keyboard_create(void) {
  lv_obj_t* kb = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(kb, editor_kb_event, LV_EVENT_ALL, NULL);
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, kb_num_map, kb_num_ctrl);
  lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
  return kb;
}

static void settings_create(lv_obj_t* parent) {
  theme_apply_view(parent);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  static lv_coord_t col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t row_dsc[] = { 40, LV_GRID_FR(1), 52, LV_GRID_TEMPLATE_LAST };
  lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);
  lv_obj_set_style_pad_column(parent, 24, 0);

  lv_obj_t* kb = numeric_keyboard_create();
  lv_obj_t* kb2 = lv_keyboard_create(lv_scr_act());
  lv_obj_add_flag(kb2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(kb2, editor_kb_event, LV_EVENT_ALL, NULL);

  edit_notes_tf = lv_textarea_create(parent);
  lv_obj_set_user_data(edit_notes_tf, (void*)"NOTES");
  theme_apply_field(edit_notes_tf);
  lv_textarea_set_max_length(edit_notes_tf, NOTES_MAX - 1);
  lv_textarea_set_one_line(edit_notes_tf, true);
  lv_textarea_set_placeholder_text(edit_notes_tf, "Notes");
  lv_obj_set_height(edit_notes_tf, 40);
  lv_obj_add_event_cb(edit_notes_tf, setting_field_changed, LV_EVENT_ALL, kb2);
  lv_obj_set_grid_cell(edit_notes_tf, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);

  // Six fields fit a column; BREW + PRIME on the left, STEAM + AUTO on the right.
  lv_obj_t* left = column_create(parent, 0, 1);
  lv_obj_t* g = group_create(left, "BREW");
  brew_temp_tf = field_create(g, "Temperature °C", kb);
  brew_pressure_tf = field_create(g, "Pressure bar", kb);
  g = group_create(left, "PRIME");
  blooming_pressure_tf = field_create(g, "Pressure bar", kb);
  blooming_fill_time_tf = field_create(g, "Fill s", kb);
  blooming_wait_time_tf = field_create(g, "Wait s", kb);

  lv_obj_t* right = column_create(parent, 1, 1);
  g = group_create(right, "STEAM");
  steam_temp_tf = field_create(g, "Temperature °C", kb);
  steam_max_pressure_tf = field_create(g, "Max pressure bar", kb);
  steam_pump_output_perc_tf = field_create(g, "Assist pump %", kb);
  steam_shot_tf = field_create(g, "Assist shot s", kb);
  steam_gap_tf = field_create(g, "Assist gap s", kb);
  g = group_create(right, "AUTO");
  brew_timer_tf = field_create(g, "Brew s", kb);

  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(row, GAP, 0);
  lv_obj_set_grid_cell(row, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 2, 1);
  setBtn = view_btn_create(row, "Save", setButtonClicked);
  view_btn_create(row, "Cancel", cancelButtonClicked);
  clearLogsBtn = view_btn_create(row, "Clear logs", clearLogsBtnClicked);
}

static void advancedSettings_create(lv_obj_t* parent) {
  theme_apply_view(parent);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
  static lv_coord_t col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
  static lv_coord_t row_dsc[] = { LV_GRID_FR(1), 52, LV_GRID_TEMPLATE_LAST };
  lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);
  lv_obj_set_style_pad_column(parent, 24, 0);

  lv_obj_t* kb = numeric_keyboard_create();

  lv_obj_t* left = column_create(parent, 0, 0);
  lv_obj_t* g = group_create(left, "BOILER");
  boiler_bb_range_tf = field_create(g, "Bang-bang band °C", kb);
  boiler_PID_cycle_tf = field_create(g, "PID cycle ms", kb);
  boiler_PID_KP_tf = field_create(g, "Kp", kb);
  boiler_PID_KI_tf = field_create(g, "Ki", kb);
  boiler_PID_KD_tf = field_create(g, "Kd", kb);

  lv_obj_t* right = column_create(parent, 1, 0);
  g = group_create(right, "PUMP");
  pump_max_step_up_tf = field_create(g, "Max step up", kb);
  pump_KP_tf = field_create(g, "Kp", kb);
  pump_KI_tf = field_create(g, "Ki", kb);
  pump_KD_tf = field_create(g, "Kd", kb);
  unused1_tf = field_create(g, "Unused", kb);

  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_column(row, GAP, 0);
  lv_obj_set_grid_cell(row, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 1, 1);
  advancedSetBtn = view_btn_create(row, "Save", advancedSetButtonClicked);
  view_btn_create(row, "Cancel", advancedCancelButtonClicked);
  view_btn_create(row, "WiFi", wifi_btn_clicked);
}

// #endif
