#include "leds.h"

#if LED_DEBUG

#include "attiny.h"
#include "io.h"

void leds_startPre() {
  LED_ON(SR_PIN_LED_ON);
  delay(FLASH_DELAY);
  leds(0, 0, 1);
  delay(FLASH_DELAY);
  leds(0, 1, 1);
  delay(FLASH_DELAY);
  leds(1, 1, 1);
  delay(FLASH_DELAY);
}

void leds_startPost() {
  for (int i = 0; i < 3; i++) {
    leds(1, 1, 0);
    delay(FLASH_DELAY);

    leds(0, 0, 0);
    delay(FLASH_DELAY);
  }
  shift();
}

void leds(bool tx, bool rx, bool err) {
  if (tx) {
    LED_ON(SR_PIN_LED_TX);
  } else {
    LED_OFF(SR_PIN_LED_TX);
  }

  if (rx) {
    LED_ON(SR_PIN_LED_RX);
  } else {
    LED_OFF(SR_PIN_LED_RX);
  }

  if (err) {
    LED_ON(SR_PIN_LED_ERR);
  } else {
    LED_OFF(SR_PIN_LED_ERR);
  }

  shift();
}

#else

void leds(bool tx, bool rx, bool err) {}

#endif  // LED_DEBUG