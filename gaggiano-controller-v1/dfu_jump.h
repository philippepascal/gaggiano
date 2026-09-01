// Reboot into the STM32 ROM bootloader (DFU) without touching BOOT0/NRST.
// See docs/FLASH-STM32.md. The button sequence keeps working regardless.
#ifndef DFU_JUMP_H
#define DFU_JUMP_H

#define ENABLE_SERIAL_DFU 1

// Sets the marker and resets the MCU. Never returns.
void dfu_request_reboot();

#endif
