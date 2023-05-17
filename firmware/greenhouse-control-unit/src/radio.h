#pragma once

#if RADIO_EN

namespace greenhouse {
namespace radio {

enum MotorDirection { k_windowExtend, k_windowRetract };

void init();
void update();
float soilTempResult();
void windowActuatorRunAll(MotorDirection direction, int runtime);
void windowActuatorSetup(int left, int right);
void switchPump(bool on);

} // namespace radio
} // namespace greenhouse

#endif // RADIO_EN
