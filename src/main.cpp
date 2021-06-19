#include "../lib/embedded/ArduinoImpl.h"
#include "../lib/embedded/BlynkImpl.h"
#include "../lib/embedded/SensorsImpl.h"
#include "../lib/greenhouse/Greenhouse.h"

GreenhouseArduino gh;
Greenhouse greenhouse(gh);

void setup()
{
    greenhouse.Setup();
}

void loop()
{
    greenhouse.Loop();
}
