// Pins, periods and limits for the STM32F411 controller. Single place to edit.
#pragma once

#define FIRMWARE_VERSION "controller-2026-09-06-r1"  // sent in HELLO: no commas, max 31 chars

#define LOOP_PERIOD_MS 10

// --- sensors
#define PRESSURE_READ_PERIOD_MS 10
#define PRESSURE_RETRY_PERIOD_MS 250   // while I2C reads fail: each attempt costs an I2C timeout (build_opt.h)
#define PRESSURE_STALE_MS 500          // no good reading for this long: the pump stays off
#define I2C_RECOVER_AFTER_FAULTS 8     // consecutive failed reads before the bus is clocked free and the ADC re-initialised
#define TEMP_READ_PERIOD_MS 250
#define I2C_SDA_PIN PB7
#define I2C_SCL_PIN PB6
#define ADS1115_ADDRESS 0x48
#define MAX6675_CS PA6
#define MAX6675_SO PB4
#define MAX6675_SCK PA5

// --- outputs
#define VALVE_PIN PC13
#define PUMP_ZC_PIN PA15
#define PUMP_DIMMER_PIN PB3
#define PUMP_ZC_MODE RISING
#define PUMP_RANGE 127
#define PUMP_MAX 255
#define STEAM_PUMP_MAX_PERCENT 50    // cap on the pump while steaming (solenoid closed)
#define STEAM_OPEN_BB_RANGE 3.0      // steaming with the wand open: heater flat out below setpoint minus this (the normal band is boiler_bb_range)
#define STEAM_ASSIST_BLANK_S 0.3     // pressure ignored for this long after a shot: the pump's own pulses raise it
#define BOILER_RELAY_PIN PB5
#define BOILER_RELAY_FREQ_HZ 30

// --- boiler PID
#define BOILER_OUTPUT_MIN 0
#define BOILER_OUTPUT_MAX 100
#define BOILER_KP 10
#define BOILER_KI .2
#define BOILER_KD .1
#define BOILER_ELEMENT_W 1400      // heating element, for the brew feed-forward
#define BREW_INLET_C 20            // reservoir water temperature assumed by the feed-forward
#define BREW_BOOST_MAX_PCT 40      // cap on the feed-forward; the PID does the rest

// --- screen link (USART2)
#define SCREEN_RX_PIN PA3
#define SCREEN_TX_PIN PA2
#define SCREEN_BAUD 115200
#define STATUS_SEND_PERIOD_MS 200  // 200 is a decent value for screen updates; last known working value was 500
#define LINK_TIMEOUT_MS 3000       // no valid CMD for this long: pump off, valve closed, boiler keeps its setpoint
#define LINE_MAX 128               // console line buffer

// --- watchdog: reset if the loop stalls (outputs go to reset state: all off)
#define WATCHDOG_TIMEOUT_US 4000000

// --- USB console
#define CONSOLE_BAUD 9600

// --- operating modes (values are part of the wire protocol, see GaggiaProtocol GpMode)
#define OPERATING_MODE_OFF 0
#define OPERATING_MODE_BREW 1
#define OPERATING_MODE_STEAM 2
#define OPERATING_MODE_CLEAN 3
