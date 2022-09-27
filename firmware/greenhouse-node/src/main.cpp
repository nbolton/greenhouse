#include "leds.h"
#include "motor.h"
#include "radio.h"
#include "sr.h"

void setup() {
#if MOTOR_EN
  motor_init();
#endif  // MOTOR_EN

#if SR_EN
  sr_init();
#endif  // SR_EN

#if LED_DEBUG
  leds_startPre();
#endif  // LED_DEBUG

#if RADIO_EN
  radio_init();
#endif  // RADIO_EN

#if LED_DEBUG
  leds_startPost();
#endif  // LED_DEBUG
}

void loop() {
#if RADIO_EN
  radio_loop();
#endif  // RADIO_EN

#if MOTOR_EN
  motor_loop();
#endif  // MOTOR_EN
}