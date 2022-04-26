#pragma once

#include "../../native/greenhouse/System.h"

#include "../Log.h"
#include "Time.h"
#include "Heating.h"
#include "Power.h"

#include <ADS1115_WE.h>
#include <Arduino.h>

class Adafruit_SHT31;

namespace embedded {
namespace greenhouse {

const int k_switchCount = 4;
struct ADC;

enum LedFlashTimes {
  k_ledRefresh = 1,
  k_ledSend = 2,
  k_ledRecieve = 3,
  k_ledStarted = 4,
  k_ledRestart = 5
};

class System : public native::greenhouse::System, ISystem {
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
  void QueueRefresh() { m_refreshQueued = true; }
  void QueueToggleActiveSwitch();

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

  void FlashLed(LedFlashTimes times);
  bool ReadSensors(int &failures);
  bool ReadSoilMoistureSensor();
  void RunWindowActuator(bool extend);
  void StopActuator();
  void Delay(unsigned long ms, const char* reason);
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
  void RefreshRate(int value);
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
  bool m_lastWriteDone;
  float m_soilMoisture;
  bool m_insideHumidityWarningSent;
  bool m_soilMoistureWarningSent;
  int m_activeSwitch;
  bool m_switchState[k_switchCount];
  bool m_refreshQueued;
  int m_blynkFailures;
  unsigned long m_lastBlynkFailure;
  std::queue<int> m_toggleActiveSwitchQueue;
  bool m_shiftRegisterEnabled;
};

} // namespace greenhouse
} // namespace embedded
