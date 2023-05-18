#include "common.h"

#include <Arduino.h>

void halt()
{
  led(LOW);

  while (1) {
    delay(1);
  }
}

void led(int value) { digitalWrite(LED_BUILTIN, value); }
