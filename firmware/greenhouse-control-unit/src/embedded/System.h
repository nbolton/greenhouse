#pragma once

#include "native/System.h"
#include <trace.h>

#include "Heating.h"
#include "Power.h"
#include "Time.h"
#include "radio.h"

#include <Arduino.h>

class Adafruit_SHT31;

namespace greenhouse {
namespace embedded {

struct ADC;

enum LedFlashTimes { k_ledRefresh = 1, k_ledStarted = 2, k_ledRestart = 3 };

class System : public greenhouse::native::System, ISystem {

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
  void Refresh() { Refresh(false); }
  void Refresh(bool first);
  void WeatherCallback();
  void OnLastWrite();
  void OnSystemStarted();
  void Restart();
  void OnPowerSwitch();
  void WriteOnboardIO(uint8_t pin, uint8_t value);
  void ApplyRefreshRate();
  void QueueCallback(CallbackFunction f, std::string name);
  void WindowSpeedUpdate();
  void ReportPumpSwitch(bool pumpOn);
  void ReportPumpStatus(const char *message);
  void SwitchLights(bool on);
  void OnBatteryCurrentChange();
  void ResetRelay();
  void ResetNodes();

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
  void ReportPower();

  void FlashLed(LedFlashTimes times);
  bool ReadSensors(int &failures);
  void UpdateSoilTemps(bool force);
  void RunWindowActuator(bool extend, float delta);
  void Delay(unsigned long ms, const char *reason);
  bool UpdateWeatherForecast();
  void HandleNightToDayTransition();
  void HandleDayToNightTransition();

public:
  // getters & setters
  greenhouse::embedded::Time &Time() { return m_time; }
  greenhouse::native::Heating &Heating() { return m_heating; }
  greenhouse::embedded::Power &Power() { return m_power; }
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
  void InitSensors();
  void UpdateCaseFan();
  void PrintCommands();
  void PrintStatus();

private:
  greenhouse::embedded::Time m_time;
  greenhouse::embedded::Heating m_heating;
  greenhouse::embedded::Power m_power;
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
  bool m_relayAlive;
};

} // namespace embedded
} // namespace greenhouse
