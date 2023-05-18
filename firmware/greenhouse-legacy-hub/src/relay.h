#pragma once

namespace legacy {

class NodeRadio;
class PumpRadio;

} // namespace legacy

namespace relay {

void init(legacy::NodeRadio &nr, legacy::PumpRadio &pr);
void update();
void txPumpStatus(const char *stauts);
void txPumpSwitch(bool on);

} // namespace relay
