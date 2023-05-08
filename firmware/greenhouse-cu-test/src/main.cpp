#include <Arduino.h>
#include <trace.h>

#include "command.h"
#include "io.h"
#include "power.h"
#include "sensors.h"

int i = 0;

void setup(void)
{
  bool ok = true;
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  while (!Serial) {
    delay(1);
  }

  TRACE(TRACE_PRINT, "Greenhouse Control Unit Test");

  // power inits IO
  if (!power::init()) {
    ok = false;
  }

  if (!sensors::init()) {
    ok = false;
  }

  if (!ok) {
    TRACE(TRACE_ERROR, "init failed");
    while (1) {
      delay(10);
    }
  }

  io::beep(2);

  command::prompt();
}

void loop(void)
{
  command::processSerial();
  if (power::update()) {
    command::prompt();
  }
}
