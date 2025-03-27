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
                     char* (*lp)(),
                     char* (*gcp)(),
                     int (*wcp)(const char* profileName),
                     int (*setupAndReadConfigFile)());
  void updateUI();
  /**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_WIDGETS_H*/
