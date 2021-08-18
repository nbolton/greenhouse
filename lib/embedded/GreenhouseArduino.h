#pragma once

#include "Greenhouse.h"

#include "ArduinoLog.h"

#include <ADS1115_WE.h>
#include <Arduino.h>

const int k_switchCount = 4;

class Adafruit_SHT31;
struct ADC;

class GreenhouseArduino : public Greenhouse {
public:
  static GreenhouseArduino &Instance();
  static void Instance(GreenhouseArduino &ga);

  GreenhouseArduino();

  void Setup();
  void Loop();
  bool Refresh();
  void RelayCallback();

  const ArduinoLog &Log() const { return m_log; }
  void TraceFlash(const __FlashStringHelper *f) const { m_log.TraceFlash(f); }

  void HandleLastWrite();
  void HandleSystemStarted();
  void HandleAutoMode(bool);
  void HandleOpenStart(float);
  void HandleOpenFinish(float);
  void HandleRefreshRate(int);
  void HandleWindowProgress(int);
  void HandleReset(int);
  void HandleRefresh(int);
  void HandleFakeInsideHumidity(float);
  void HandleFakeSoilTemperature(float);
  void HandleFakeSoilMoisture(float);
  void HandleTestMode(int);
  void HandleOpenDayMinimum(int);
  void HandleInsideHumidityWarning(float);
  void HandleSoilMostureWarning(float);
  void HandleActiveSwitch(int);
  void HandleToggleSwitch(int);
  void HandleDayStartHour(int param) { DayStartHour(param); }
  void HandleDayEndHour(int param) { DayEndHour(param); }
  void HandlePvVoltageSensorMin(float param) { m_pvVoltageSensorMin = param; }
  void HandlePvVoltageSensorMax(float param) { m_pvVoltageSensorMax = param; }
  void HandlePvVoltageOutputMin(float param) { m_pvVoltageOutputMin = param; }
  void HandlePvVoltageOutputMax(float param) { m_pvVoltageOutputMax = param; }
  void HandlePvCurrentSensorMin(float param) { m_pvCurrentSensorMin = param; }
  void HandlePvCurrentSensorMax(float param) { m_pvCurrentSensorMax = param; }
  void HandlePvCurrentOutputMin(float param) { m_pvCurrentOutputMin = param; }
  void HandlePvCurrentOutputMax(float param) { m_pvCurrentOutputMax = param; }
  void HandlePvVoltageSwitchOn(float param) { m_pvVoltageSwitchOn = param; }
  void HandlePvVoltageSwitchOff(float param) { m_pvVoltageSwitchOff = param; }
  void HandlePvForceOn(int param) { m_pvForceOn = param; }
  void HandleWindowOpenSpeed(int value) { WindowActuatorSpeedPercent(value); }
  void HandleWindowOpenRuntime(float value) { WindowActuatorRuntimeSec(value); }
  void HandleDayWaterTemperature(float value) { DayWaterTemperature(value); }
  void HandleNightWaterTemperature(float value) { NightWaterTemperature(value); }
  void HandleDaySoilTemperature(float value) { DaySoilTemperature(value); }
  void HandleNightSoilTemperature(float value) { NightSoilTemperature(value); }

protected:
  void ReportInfo(const char *m, ...);
  void ReportWarning(const char *m, ...);
  void ReportCritical(const char *m, ...);
  void ReportSensorValues();
  void ReportWindowProgress();
  void ReportSystemInfo();
  void ReportWarnings();

  void FlashLed(LedFlashTimes times);
  bool ReadSensors();
  void Reset();
  void SwitchWaterHeating(bool on);
  void SwitchSoilHeating(bool on);
  void RunWindowActuator(bool forward);
  void StopActuator();
  void SetWindowActuatorSpeed(int speed);
  void SystemDelay(unsigned long ms);

  // getters & setters
  int CurrentHour() const;
  float InsideTemperature() const { return m_insideTemperature; }
  float InsideHumidity() const;
  float OutsideTemperature() const { return m_outsideTemperature; }
  float OutsideHumidity() const { return m_outsideHumidity; }
  float SoilTemperature() const;
  float SoilMoisture() const;
  float WaterTemperature() const { return m_waterTemperature; }

private:
  void StartBeep(int times);
  void Actuator(bool forward, int s, int t);
  void SetSwitch(int i, bool on);
  void ToggleActiveSwitch();
  void SwitchPower(bool pv);
  void MeasureVoltage();
  void MeasureCurrent();
  float ReadAdc(ADC &adc, ADS1115_MUX channel);
  void InitSensors();

private:
  ArduinoLog m_log;
  float m_insideTemperature;
  float m_insideHumidity;
  float m_outsideTemperature;
  float m_outsideHumidity;
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
  int m_switchState[k_switchCount];
  bool m_pvPowerSource;
  float m_pvVoltageSwitchOn = k_unknown;
  float m_pvVoltageSwitchOff = k_unknown;
  int m_pvVoltageCount = 0;
  float m_pvVoltageAverage = 0;
  float m_pvVoltageSum = 0;
  float m_pvVoltageSensor = k_unknown;
  float m_pvVoltageOutput = k_unknown;
  float m_pvVoltageSensorMin = 0;
  float m_pvVoltageSensorMax = k_unknown;
  float m_pvVoltageOutputMin = 0;
  float m_pvVoltageOutputMax = k_unknown;
  float m_pvCurrentSensor = k_unknown;
  float m_pvCurrentOutput = k_unknown;
  float m_pvCurrentSensorMin = 0;
  float m_pvCurrentSensorMax = k_unknown;
  float m_pvCurrentOutputMin = 0;
  float m_pvCurrentOutputMax = k_unknown;
  bool m_pvForceOn = false;
  bool m_waterHeatingOn = false;
  bool m_soilHeatingOn = false;
};
