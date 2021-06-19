#pragma once

#include "IBlynk.h"
#include "ISensors.h"

class Greenhouse
{
    public:
        Greenhouse(IBlynk& blynk, ISensors& sensors);
        void Run();

    private:
        IBlynk& m_blynk;
        ISensors& m_sensors;
};
