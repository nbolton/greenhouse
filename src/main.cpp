#include "GreenhouseArduino.h"

GreenhouseArduino greenhouse;

void setup()
{
  GreenhouseArduino::Instance(greenhouse);
  greenhouse.Setup();
}

void loop() { greenhouse.Loop(); }
