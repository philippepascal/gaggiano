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

/*********************
 *      DEFINES
 *********************/
struct GaggiaState {
  double boilerSetPoint;
  double tempRead;
  double pressureSetPoint;
  double pressureRead;
  boolean isBoilerOn;
  boolean isBrewing;
  boolean isSteaming;
};
/**********************
 *      TYPEDEFS
 **********************/
typedef struct GaggiaState GaggiaStateT;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void instantiateUI(GaggiaStateT* state);
void updateUI();
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_WIDGETS_H*/
