#pragma once

#if PUMP_RADIO_EN

namespace legacy {

class PumpRadio {
public:
  PumpRadio();
  void Init();
  void Update();
  bool SwitchPump(bool on);

private:
  bool SendPumpMessage();
};

} // namespace legacy

#endif // PUMP_RADIO_EN
