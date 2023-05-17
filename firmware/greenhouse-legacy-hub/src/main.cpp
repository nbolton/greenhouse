#include <Arduino.h>

#include "hc12.h"
#include "s_rf95.h"
#include "s_rf69.h"

void setup()
{
  hc12::init();
  rf95::init();
  rf69::init();
}

void loop()
{
  hc12::update();
  rf95::update();
  rf69::update();
}
