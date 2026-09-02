/**
 * @file lv_buildUI.h
 *
 */

#ifndef LV_DEMO_WIDGETS_H
#define LV_DEMO_WIDGETS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
//#include "../lv_demos.h"
//adding arduino for using serial.print
#include "Arduino.h"
#include <stddef.h>
#include <lvgl.h>
#include "gaggia_state.h"

  /*********************
 *      DEFINES
 *********************/
  /**********************
 *      TYPEDEFS
 **********************/

  /**********************
 * GLOBAL PROTOTYPES
 **********************/
  void instantiateUI(GaggiaStateT* state,
                     AdvancedSettingsT* advancedSettings,
                     int (*writeConfigFile)(),
                     int (*lp)(char* buf, size_t size),
                     int (*gcp)(char* buf, size_t size),
                     int (*wcp)(const char* profileName),
                     int (*setupAndReadConfigFile)(),
                     int (*renameProfile)(const char* newName),
                     bool (*deleteProfile)(const char* profileToDelete),
                     int (*duplicateProfile)());
  void updateUI();
  void ui_show_notice(const char* text);
  void ui_hide_notice(void);
  /**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_WIDGETS_H*/
