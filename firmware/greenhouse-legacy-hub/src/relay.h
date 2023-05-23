#pragma once

#include <radio_common.h>

namespace legacy {

class NodeRadio;
class PumpRadio;

} // namespace legacy

namespace relay {

namespace radio = greenhouse::radio;

void init(legacy::NodeRadio &nr, legacy::PumpRadio &pr);
void update();
bool txPumpStatus(radio::PumpStatus status);
bool txPumpSwitch(bool on);

} // namespace relay
