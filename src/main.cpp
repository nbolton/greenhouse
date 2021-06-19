#include "../lib/greenhouse/embedded/BlynkImpl.h"
#include "../lib/greenhouse/embedded/SensorsImpl.h"
#include "../lib/greenhouse/Greenhouse.h"

#include <Arduino.h>

BlynkImpl blynk;
SensorsImpl sensors;
Greenhouse greenhouse(blynk, sensors);

void setup()
{
    
}

void loop()
{
}
