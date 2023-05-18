#pragma once

#if PUMP_RADIO_EN

namespace legacy {

class PumpRadio {
public:
  PumpRadio();
  void Init();
  void Update();
  void SwitchPump(bool on);

private:
  void SendPumpMessage();
};

} // namespace legacy

#endif // PUMP_RADIO_EN
