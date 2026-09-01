// Reboot into the STM32F411 system bootloader from firmware.
//
// How it works: dfu_request_reboot() writes a two-word marker into a RAM
// variable that the startup code does not clear (.noinit section), then does a
// software reset. On the next boot, a constructor with priority 100 runs before
// the STM32duino core's premain() (priority 101), i.e. before clocks, HAL and
// USB are configured. If the marker is present it is cleared and execution jumps
// to system memory at 0x1FFF0000, exactly as if BOOT0 had been held at reset.
// The marker cannot survive a power cycle in a meaningful way (SRAM is random
// at power-on and both words must match), so a plain power-on never enters DFU.
#include "dfu_jump.h"
#include <Arduino.h>

#if ENABLE_SERIAL_DFU

#define DFU_MAGIC_A 0xD0F0B007u
#define DFU_MAGIC_B 0x5EC0DE42u
#define SYSTEM_MEMORY_BASE 0x1FFF0000u  // STM32F411 (AN2606)

__attribute__((section(".noinit"))) static volatile uint32_t dfu_marker_a;
__attribute__((section(".noinit"))) static volatile uint32_t dfu_marker_b;

void dfu_request_reboot() {
  dfu_marker_a = DFU_MAGIC_A;
  dfu_marker_b = DFU_MAGIC_B;
  __DSB();
  NVIC_SystemReset();
  for (;;) {}
}

// Priorities 0-100 are "reserved" (a warning, not an error). 100 is the only
// way to run before the core's premain() at 101, and nothing else in the
// firmware uses it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
__attribute__((constructor(100))) static void dfu_check_marker() {
  if (dfu_marker_a != DFU_MAGIC_A || dfu_marker_b != DFU_MAGIC_B) {
    return;
  }
  dfu_marker_a = 0;
  dfu_marker_b = 0;

  // At this point SystemInit() has run (RCC at reset defaults, HSI clock) and
  // nothing else. Leave the MCU as close to reset state as possible.
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL = 0;
  for (unsigned i = 0; i < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); i++) {
    NVIC->ICER[i] = 0xFFFFFFFFu;
    NVIC->ICPR[i] = 0xFFFFFFFFu;
  }
  // Map system memory at address 0 like the BOOT0 pin would.
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  SYSCFG->MEMRMP = SYSCFG_MEMRMP_MEM_MODE_0;
  __DSB();
  __ISB();

  uint32_t stack = *(volatile uint32_t *)SYSTEM_MEMORY_BASE;
  uint32_t entry = *(volatile uint32_t *)(SYSTEM_MEMORY_BASE + 4);
  __set_MSP(stack);
  ((void (*)(void))entry)();
  for (;;) {}
}
#pragma GCC diagnostic pop

#else
void dfu_request_reboot() {}
#endif
