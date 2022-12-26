#pragma once

#include "../../common/log.h"
#include "../../native/greenhouse/System.h"

#include "Heating.h"
#include "Power.h"
#include "PumpRadio.h"
#include "Radio.h"
#include "Time.h"

#include <ADS1115_WE.h>
#include <Arduino.h>

class Adafruit_SHT31;

namespace embedded {
namespace greenhouse {

struct ADC;

enum LedFlashTimes { k_ledRefresh = 1, k_ledStarted = 2, k_ledRestart = 3 };

class System : public native::greenhouse::System, ISystem {

  typedef void (System::*CallbackFunction)(void);

  struct Callback {
    CallbackFunction m_function;
    std::string m_name;
    Callback(CallbackFunction function, std::string name) : m_function(function), m_name(name) {}
  };

public:
  static System &Instance();
  static void Instance(System &ga);

  System();

  void Setup();
  void Loop();
  void Refresh();
  void WeatherCallback();
  void OnLastWrite();
  void OnSystemStarted();
  void Restart();
  void OnPowerSwitch();
  void WriteOnboardIO(uint8_t pin, uint8_t value);
  bool BatterySensorReady();
  float ReadBatteryVoltageSensorRaw();
  float ReadBatteryCurrentSensorRaw();
  void ApplyRefreshRate();
  void QueueCallback(CallbackFunction f, std::string name);
  void WindowSpeedUpdate();
  void LowerPumpOn(bool pumpOn);
  void LowerPumpStatus(const char *message);
  void SwitchLights(bool on);

protected:
  void ReportInfo(const char *m, ...);
  void ReportWarning(const char *m, ...);
  void ReportCritical(const char *m, ...);
  void ReportSensorValues();
  void ReportWindowOpenPercent();
  void ReportSystemInfo();
  void ReportWarnings();
  void ReportWeather();
  void ReportWaterHeaterInfo();

  void FlashLed(LedFlashTimes times);
  bool ReadSensors(int &failures);
  void UpdateSoilTemp();
  void RunWindowActuator(bool extend, float delta);
  void Delay(unsigned long ms, const char *reason);
  bool UpdateWeatherForecast();
  void HandleNightToDayTransition();
  void HandleDayToNightTransition();

public:
  // getters & setters
  embedded::greenhouse::Time &Time() { return m_time; }
  native::greenhouse::Heating &Heating() { return m_heating; }
  embedded::greenhouse::Power &Power() { return m_power; }
  embedded::greenhouse::PumpRadio &PumpRadio() { return m_pumpRadio; }
  float InsideAirTemperature() const { return m_insideAirTemperature; }
  float InsideAirHumidity() const;
  float OutsideAirTemperature() const { return m_outsideAirTemperature; }
  float OutsideAirHumidity() const { return m_outsideAirHumidity; }
  float SoilTemperature() const;
  float WaterTemperature() const { return m_waterTemperature; }
  void RefreshRate(int value) { m_refreshRate = value; }
  void FakeInsideHumidity(float value) { m_fakeInsideHumidity = value; }
  void FakeSoilTemperature(float value) { m_fakeSoilTemperature = value; }
  bool RefreshBusy() const { return m_refreshBusy; }
  int WindowSpeedLeft() const { return m_windowSpeedLeft; }
  void WindowSpeedLeft(int value) { m_windowSpeedLeft = value; }
  int WindowSpeedRight() const { return m_windowSpeedRight; }
  void WindowSpeedRight(int value) { m_windowSpeedRight = value; }

private:
  void InitIO();
  void Beep(int times, bool longBeep);
  void CaseFan(bool on);
  void Actuator(bool forward, int s, int t);
  float ReadAdc(ADC &adc, ADS1115_MUX channel);
  void InitSensors();
  void InitExternalADC();
  void UpdateCaseFan();
  void PrintCommands();
  void PrintStatus();
  void ReportPower();

private:
  embedded::greenhouse::Time m_time;
  embedded::greenhouse::Heating m_heating;
  embedded::greenhouse::Power m_power;
  embedded::greenhouse::PumpRadio m_pumpRadio;
  float m_insideAirTemperature;
  float m_insideAirHumidity;
  float m_outsideAirTemperature;
  float m_outsideAirHumidity;
  float m_soilTemperature;
  int m_soilTemperatureFailures;
  float m_waterTemperature;
  int m_timerId;
  int m_led;
  float m_fakeInsideHumidity;
  float m_fakeSoilTemperature;
  bool m_refreshBusy;
  bool m_insideHumidityWarningSent;
  int m_blynkFailures;
  unsigned long m_lastBlynkFailure;
  int m_refreshRate;
  std::queue<Callback> m_callbackQueue;
  bool m_queueOnSystemStarted;
  unsigned long m_lastLoop;
  int m_windowSpeedLeft;
  int m_windowSpeedRight;
#if RADIO_EN
  Radio m_radio;
#endif // RADIO_EN
};

} // namespace greenhouse
} // namespace embedded
