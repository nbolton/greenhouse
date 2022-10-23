#include "leds.h"
#include "motor.h"
#include "radio.h"
#include "io.h"
#include "temp.h"

void setup() {
#if IO_EN
  io_init();
#endif  // IO_EN

#if LED_DEBUG
  leds_startPre();
#endif  // LED_DEBUG

#if MOTOR_EN
  motor_init();
#endif  // MOTOR_EN

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

#if TEMP_EN
  temp_loop();
#endif  // TEMP_EN
}
