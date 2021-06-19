#pragma once

#include "IGreenhouseArduino.h"

class Greenhouse
{
    public:
        Greenhouse(IGreenhouseArduino& ga);
        void Setup();
        void Loop();

    private:
        IGreenhouseArduino& m_ga;
};
