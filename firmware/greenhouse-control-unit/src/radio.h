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
bool isRelayAlive();
float getSoilTempsResult();
bool txKeepAliveRelay();
void txGetSoilTempsAsync();
void txGetRelayStatusAsync();
void txWindowActuatorRun(MotorDirection direction, int runtime, Window window);
void txWindowActuatorSetup(int speed, Window window);
void txPumpSwitch(bool on);

} // namespace radio
} // namespace greenhouse

#endif // RADIO_EN
