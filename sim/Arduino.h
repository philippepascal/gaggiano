// Host shim for the simulator: the UI code and lv_conf.h only need millis().
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint32_t millis(void);
#ifdef __cplusplus
}
#endif
