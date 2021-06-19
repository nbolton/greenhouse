#include "Greenhouse.h"

#include <stdio.h>

Greenhouse::Greenhouse(IGreenhouseArduino& ga) :
  m_ga(ga)
{
}

void Greenhouse::Setup()
{
  m_ga.Setup();
}

void Greenhouse::Loop()
{
  m_ga.Loop();
}
