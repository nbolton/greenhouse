#pragma once

#if PUMP_RADIO_EN

namespace legacy {

class PumpRadio {
public:
  PumpRadio();
  void Init();
  void Update();
  void SendPumpMessage();
  void SwitchPump(bool on);

private:
  void LowerPumpStatus(const char *status) {}
  void LowerPumpOn(bool isOn) {}
};

} // namespace legacy

#endif // PUMP_RADIO_EN
