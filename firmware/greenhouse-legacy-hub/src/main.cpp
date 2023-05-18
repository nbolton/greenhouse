#include <Arduino.h>

#include "common.h"
#include "legacy/NodeRadio.h"
#include "legacy/PumpRadio.h"
#include "relay.h"

using namespace legacy;

NodeRadio nr;
PumpRadio pr;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  led(LOW);

  relay::init(nr, pr);
  nr.Init();
  pr.Init();

  led(HIGH);
}

void loop()
{
  relay::update();
  nr.Update();
  pr.Update();
}
