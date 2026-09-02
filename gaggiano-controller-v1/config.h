// Pins, periods and limits for the STM32F411 controller. Single place to edit.
#pragma once

#define FIRMWARE_VERSION "controller-2026-09-01-r2"  // sent in HELLO: no commas, max 31 chars

#define LOOP_PERIOD_MS 10

// --- sensors
#define PRESSURE_READ_PERIOD_MS 10
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
#define BOILER_RELAY_PIN PB5
#define BOILER_RELAY_FREQ_HZ 30

// --- boiler PID
#define BOILER_OUTPUT_MIN 0
#define BOILER_OUTPUT_MAX 100
#define BOILER_KP 10
#define BOILER_KI .2
#define BOILER_KD .1

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
