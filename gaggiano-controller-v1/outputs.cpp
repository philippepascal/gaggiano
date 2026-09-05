#include "outputs.h"
#include "config.h"
#include "PSM.h"

double boiler_relay_output = 0;
double pump_dimmer_output2 = 0;
volatile uint32_t zeroCrossings = 0;

// PSM calls this from the zero-cross interrupt (weak hook in PSM.cpp).
void onPSMInterrupt() {
  zeroCrossings++;
}

static PSM *pump;
static uint32_t boiler_relay_pin_channel;  // timer channel for the boiler pin
static HardwareTimer *boilerTimer;

void outputsSetup() {
  pinMode(VALVE_PIN, OUTPUT);
  digitalWrite(VALVE_PIN, LOW);  // just in case

  pump = new PSM(PUMP_ZC_PIN, PUMP_DIMMER_PIN, PUMP_RANGE, PUMP_ZC_MODE, 2);
  pump->set(0);

  pinMode(BOILER_RELAY_PIN, OUTPUT);
  // Retrieve the TIM instance and channel associated to the pin, then drive it
  // as PWM at BOILER_RELAY_FREQ_HZ. 'new' so the object outlives setup().
  TIM_TypeDef *instance = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(BOILER_RELAY_PIN), PinMap_PWM);
  boiler_relay_pin_channel = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(BOILER_RELAY_PIN), PinMap_PWM));
  boilerTimer = new HardwareTimer(instance);
  boilerTimer->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, BOILER_RELAY_FREQ_HZ, 0);
}

void setBoilerOutput(double percent) {
  boilerTimer->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, BOILER_RELAY_FREQ_HZ, percent);
}

void setPump(double value) {
  pump_dimmer_output2 = value;
  pump->set(value);
}

void setValve(bool open) {
  digitalWrite(VALVE_PIN, open ? HIGH : LOW);
}

bool valveIsOpen() {
  return digitalRead(VALVE_PIN) == HIGH;
}
