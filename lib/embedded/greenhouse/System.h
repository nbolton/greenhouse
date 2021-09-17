#pragma once

#include "../../native/greenhouse/System.h"

#include "../Log.h"
#include "Heating.h"

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

enum PvModes { k_pvAuto, k_pvOn, k_pvOff };

class System : public native::greenhouse::System {
public:
  static System &Instance();
  static void Instance(System &ga);

  System();

  void Setup();
  void Loop();
  bool Refresh();
  void RelayCallback();
  void WeatherCallback();
  void OnLastWrite();
  void OnSystemStarted();
  void Restart();
  void ToggleActiveSwitch();
  void HandleWindowProgress(int value);
  void ManualRefresh();
  void SetSwitch(int index, bool on);

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

  void FlashLed(LedFlashTimes times);
  bool ReadSensors(int &failures);
  void RunWindowActuator(bool forward);
  void StopActuator();
  void SetWindowActuatorSpeed(int speed);
  void SystemDelay(unsigned long ms);
  bool UpdateWeatherForecast();
  void HandleNightDayTransition();

public:
  // getters & setters
  native::greenhouse::Heating &Heating() { return m_heating; }
  int CurrentHour() const;
  unsigned long UptimeSeconds() const;
  unsigned long EpochTime() const;
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
  void PvVoltageSensorMin(float value) { m_pvVoltageSensorMin = value; }
  void PvVoltageSensorMax(float value) { m_pvVoltageSensorMax = value; }
  void PvVoltageOutputMin(float value) { m_pvVoltageOutputMin = value; }
  void PvVoltageOutputMax(float value) { m_pvVoltageOutputMax = value; }
  void PvCurrentSensorMin(float value) { m_pvCurrentSensorMin = value; }
  void PvCurrentSensorMax(float value) { m_pvCurrentSensorMax = value; }
  void PvCurrentOutputMin(float value) { m_pvCurrentOutputMin = value; }
  void PvCurrentOutputMax(float value) { m_pvCurrentOutputMax = value; }
  void PvVoltageSwitchOn(float value) { m_pvVoltageSwitchOn = value; }
  void PvVoltageSwitchOff(float value) { m_pvVoltageSwitchOff = value; }
  PvModes PvMode() const { return m_pvMode; }
  void PvMode(PvModes value) { m_pvMode = value; }

private:
  void StartBeep(int times);
  void CaseFan(bool on);
  void Actuator(bool forward, int s, int t);
  void SwitchPower(bool pv);
  void MeasureVoltage();
  void MeasureCurrent();
  float ReadAdc(ADC &adc, ADS1115_MUX channel);
  void InitSensors();
  void InitADCs();
  void InitActuators();
  void InitShiftRegisters();
  void InitPowerSource();
  void UpdateTime();

private:
  embedded::Log m_log;
  embedded::greenhouse::Heating m_heating;
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
  bool m_pvPowerSource;
  float m_pvVoltageSwitchOn;
  float m_pvVoltageSwitchOff;
  float m_pvVoltageSensor;
  float m_pvVoltageOutput;
  float m_pvVoltageSensorMin;
  float m_pvVoltageSensorMax;
  float m_pvVoltageOutputMin;
  float m_pvVoltageOutputMax;
  float m_pvCurrentSensor;
  float m_pvCurrentOutput;
  float m_pvCurrentSensorMin;
  float m_pvCurrentSensorMax;
  float m_pvCurrentOutputMin;
  float m_pvCurrentOutputMax;
  PvModes m_pvMode;
  bool m_timeClientOk;
};

} // namespace greenhouse
} // namespace embedded
