#include "../lib/embedded/ArduinoImpl.h"
#include "../lib/embedded/BlynkImpl.h"
#include "../lib/embedded/SensorsImpl.h"
#include "../lib/greenhouse/Greenhouse.h"

GreenhouseArduino greenhouse;

void setup()
{
    GreenhouseArduino::Instance(greenhouse);
    greenhouse.Setup();
}

void loop()
{
    greenhouse.Loop();
}
