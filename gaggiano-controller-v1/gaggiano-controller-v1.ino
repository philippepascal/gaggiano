// Gaggiano controller: STM32F411 "Black Pill". Reads boiler temperature and
// pressure, drives the boiler SSR, the pump and the solenoid valve, and talks
// to the touch screen over USART2. See docs/ in the repository root.
//
// Modules: config.h (pins, periods), sensors, outputs, control, link (screen
// serial protocol), console (USB commands), dfu_jump (reboot to bootloader).

#include "config.h"
#include "sensors.h"
#include "outputs.h"
#include "control.h"
#include "link.h"
#include "console.h"
#include <IWatchdog.h>

uint32_t loopCounter = 0;
uint32_t maxLoopMs = 0;             // since boot, for the console
static uint32_t statMaxLoopMs = 0;  // since the last STAT, goes on the wire

bool resetByWatchdog = false;

void setup() {
  resetByWatchdog = IWatchdog.isReset();
  if (resetByWatchdog) IWatchdog.clearReset();
  consoleSetup();
  if (resetByWatchdog) Serial.println("reset by watchdog");
  linkSetup();
  sensorsSetup();
  outputsSetup();
  controlSetup();
  IWatchdog.begin(WATCHDOG_TIMEOUT_US);
}

void loop() {
  uint32_t loopStart = millis();

  bool tempUpdated = readTemperature(loopStart);
  bool pressureUpdated = readPressure(loopStart);

  pollScreenSerial();
  checkLinkTimeout(loopStart);
  readUsbCommand();

  if (tempUpdated) updateBoiler();
  if (pressureUpdated) updatePump2();  // pump and solenoid (coupled)

  if (sendStatus(loopStart, loopCounter, statMaxLoopMs)) statMaxLoopMs = 0;

  loopCounter++;
  uint32_t elapsed = millis() - loopStart;
  if (elapsed > maxLoopMs) maxLoopMs = elapsed;
  if (elapsed > statMaxLoopMs) statMaxLoopMs = elapsed;
  // Only sleep for the remainder of the period (an unsigned subtraction here
  // used to turn any slow iteration into a ~49 day delay).
  if (elapsed < LOOP_PERIOD_MS) delay(LOOP_PERIOD_MS - elapsed);
  IWatchdog.reload();
}

// Hardware notes kept from the original single-file sketch:
// - USB CDC must be enabled in the build (usb=CDCgen) for Serial to exist.
//   https://www.stm32duino.com/viewtopic.php?t=1353
// - The relay on the solenoid can prevent DFU enumeration; turning brew on
//   before entering DFU helps.
// - PB5 PWM for the boiler relay follows the HardwareTimer All-in-one_setPWM example.
// - The valve relay had to be wired normally closed.
// - PSM (pump) library from https://github.com/banoz/PSM.Library.git, copied in.
