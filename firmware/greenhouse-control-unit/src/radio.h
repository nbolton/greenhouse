#pragma once

#if RADIO_EN

#include "radio_common.h"

namespace greenhouse {

namespace embedded {

class System;

}

namespace radio {

void init(embedded::System *_system);
void update();
float getSoilTempsResult();
void getSoilTempsAsync();
void windowActuatorRunAll(MotorDirection direction, int runtime);
void windowActuatorSetup(int left, int right);
void pumpSwitch(bool on);
void resetRelay();

} // namespace radio
} // namespace greenhouse

#endif // RADIO_EN
