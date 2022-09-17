#pragma once

#include "../../native/greenhouse/System.h"

#include "../Log.h"
#include "Heating.h"
#include "Power.h"
#include "Time.h"
#include "Radio.h"

#include <ADS1115_WE.h>
#include <Arduino.h>

class Adafruit_SHT31;

namespace embedded {
namespace greenhouse {

const int k_switchCount = 4;
struct ADC;

enum LedFlashTimes {
  k_ledRefresh = 1,
  k_ledStarted = 2,
  k_ledRestart = 3
};

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
  void ToggleSwitch(int switchIndex);
  void SetSwitch(int index, bool on);
  void OnPowerSwitch();
  void ExpanderWrite(int pin, int value);
  void ShiftRegister(int pin, bool set);
  bool PowerSensorReady();
  float ReadPowerSensorVoltage();
  float ReadPowerSensorCurrent();
  float ReadCommonVoltageSensor();
  float ReadPsuVoltageSensor();
  void QueueToggleActiveSwitch();
  void ApplyRefreshRate();
  void QueueCallback(CallbackFunction f, std::string name);

  const embedded::Log &Log() const { return m_log; }

protected:
  void ReportInfo(const char *m, ...);
  void ReportWarning(const char *m, ...);
  void ReportCritical(const char *m, ...);
  void ReportSensorValues();
  void ReportWindowProgress();
  void ReportSystemInfo();
  void ReportWarnings();
  void ReportWeather();
  void ReportWaterHeaterInfo();
  void ReportMoistureCalibration();
  void ReportSwitchStates();

  void FlashLed(LedFlashTimes times);
  bool ReadSensors(int &failures);
  bool ReadSoilMoistureSensor();
  void RunWindowActuator(bool extend);
  void StopActuator();
  void Delay(unsigned long ms, const char *reason);
  bool UpdateWeatherForecast();
  void HandleNightToDayTransition();
  void HandleDayToNightTransition();

public:
  // getters & setters
  embedded::greenhouse::Time &Time() { return m_time; }
  native::greenhouse::Heating &Heating() { return m_heating; }
  embedded::greenhouse::Power &Power() { return m_power; }
  float InsideAirTemperature() const { return m_insideAirTemperature; }
  float InsideAirHumidity() const;
  float OutsideAirTemperature() const { return m_outsideAirTemperature; }
  float OutsideAirHumidity() const { return m_outsideAirHumidity; }
  float SoilTemperature() const;
  float SoilMoisture() const;
  float WaterTemperature() const { return m_waterTemperature; }
  void RefreshRate(int value) { m_refreshRate = value; }
  void FakeInsideHumidity(float value) { m_fakeInsideHumidity = value; }
  void FakeSoilTemperature(float value) { m_fakeSoilTemperature = value; }
  void FakeSoilMoisture(float value) { m_fakeSoilMoisture = value; }
  void ActiveSwitch(int value) { m_activeSwitch = value; }
  bool RefreshBusy() const { return m_refreshBusy; }

private:
  void Beep(int times, bool longBeep);
  void CaseFan(bool on);
  void Actuator(bool forward, int s, int t);
  float ReadAdc(ADC &adc, ADS1115_MUX channel);
  void InitSensors();
  void InitADCs();
  void InitShiftRegisters();
  void UpdateCaseFan();
  void PrintCommands();
  void PrintStatus();

private:
  embedded::Log m_log;
  embedded::greenhouse::Time m_time;
  embedded::greenhouse::Heating m_heating;
  embedded::greenhouse::Power m_power;
  float m_insideAirTemperature;
  float m_insideAirHumidity;
  float m_outsideAirTemperature;
  float m_outsideAirHumidity;
  float m_soilTemperature;
  float m_waterTemperature;
  int m_timerId;
  int m_led;
  float m_fakeInsideHumidity;
  float m_fakeSoilTemperature;
  float m_fakeSoilMoisture;
  bool m_refreshBusy;
  float m_soilMoisture;
  bool m_insideHumidityWarningSent;
  bool m_soilMoistureWarningSent;
  int m_activeSwitch;
  bool m_switchState[k_switchCount];
  int m_blynkFailures;
  unsigned long m_lastBlynkFailure;
  std::queue<int> m_toggleActiveSwitchQueue;
  bool m_shiftRegisterEnabled;
  int m_refreshRate;
  std::queue<Callback> m_callbackQueue;
  bool m_queueOnSystemStarted;
  Radio m_radio;
};

} // namespace greenhouse
} // namespace embedded