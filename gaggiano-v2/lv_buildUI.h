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
#include "gaggia_config.h"


/*********************
 *      DEFINES
 *********************/
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void instantiateUI();
void updateUI();
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_WIDGETS_H*/
