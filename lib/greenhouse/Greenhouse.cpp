#include "Greenhouse.h"
#include <stdio.h>

Greenhouse::Greenhouse(IBlynk& blynk, ISensors& sensors) :
    m_blynk(blynk),
    m_sensors(sensors)
{
}

void Greenhouse::Run()
{
    printf("Hello world");
}
