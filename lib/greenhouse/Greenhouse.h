#pragma once

#include "Log.h"

const int k_unknown = -1;

class Greenhouse {
public:
  Greenhouse();

  virtual void Setup();
  virtual void Loop();
  virtual bool Refresh();

  virtual const ::Log &Log() const { return m_log; }

protected:
  virtual void ReportInfo(const char *m, ...) {}
  virtual void ReportWarning(const char *m, ...) {}
  virtual void ReportCritical(const char *m, ...) {}
  virtual void ReportSensorValues() {}
  virtual void ReportWindowProgress() {}
  virtual void ReportSystemInfo() {}
  virtual void ReportWarnings() {}

  virtual bool ReadSensors(int& failures) { return false; }
  virtual void OpenWindow(float delta);
  virtual void CloseWindow(float delta);
  virtual float CalculateMoisture(float analogValue) const;
  virtual bool ApplyWindowProgress(float expectedProgress);
  virtual void AddWindowProgressDelta(float delta);
  virtual void SwitchWaterHeating(bool on) {}
  virtual void SwitchSoilHeating(bool on) {}
  virtual void SwitchAirHeating(bool on) {}
  virtual void SwitchWaterHeatingIfChanged(bool on);
  virtual void SwitchSoilHeatingIfChanged(bool on);
  virtual void SwitchAirHeatingIfChanged(bool on);
  virtual void UpdateHeatingSystems();
  virtual void AdjustWindow(bool open, float delta);
  virtual void RunWindowActuator(bool forward) {}
  virtual void StopActuator() {}
  virtual void SetWindowActuatorSpeed(int speed) {}
  virtual void SystemDelay(unsigned long ms) {}

public:
  // getters & setters
  virtual int CurrentHour() const { return k_unknown; }
  virtual float InsideAirTemperature() const { return k_unknown; }
  virtual float InsideAirHumidity() const { return k_unknown; }
  virtual float OutsideAirTemperature() const { return k_unknown; }
  virtual float OutsideAirHumidity() const { return k_unknown; }
  virtual float SoilTemperature() const { return k_unknown; }
  virtual float WaterTemperature() const { return k_unknown; }
  virtual float SoilMoisture() const { return k_unknown; }
  virtual void WindowProgress(int value) { m_windowProgress = value; }
  virtual int WindowProgress() const { return m_windowProgress; }
  virtual void AutoMode(bool value) { m_autoMode = value; }
  virtual bool AutoMode() const { return m_autoMode; }
  virtual void OpenStart(float value) { m_openStart = value; }
  virtual float OpenStart() const { return m_openStart; }
  virtual void OpenFinish(float value) { m_openFinish = value; }
  virtual float OpenFinish() const { return m_openFinish; }
  virtual void TestMode(bool value) { m_testMode = value; }
  virtual bool TestMode() const { return m_testMode; }
  virtual void OpenDayMinimum(int value) { m_openDayMinimum = value; }
  virtual int OpenDayMinimum() const { return m_openDayMinimum; }
  virtual void InsideHumidityWarning(float value) { m_insideHumidityWarning = value; }
  virtual float InsideHumidityWarning() const { return m_insideHumidityWarning; }
  virtual void SoilMostureWarning(float value) { m_soilMostureWarning = value; }
  virtual float SoilMostureWarning() const { return m_soilMostureWarning; }
  virtual void DayStartHour(int value) { m_dayStartHour = value; }
  virtual int DayStartHour() const { return m_dayStartHour; }
  virtual void DayEndHour(int value) { m_dayEndHour = value; }
  virtual int DayEndHour() const { return m_dayEndHour; }
  virtual void WindowActuatorSpeedPercent(int value) { m_windowActuatorSpeedPercent = value; }
  virtual int WindowActuatorSpeedPercent() const { return m_windowActuatorSpeedPercent; }
  virtual void WindowActuatorRuntimeSec(float value) { m_windowActuatorRuntimeSec = value; }
  virtual int WindowActuatorRuntimeSec() const { return m_windowActuatorRuntimeSec; }
  virtual void DayWaterTemperature(float value) { m_dayWaterTemperature = value; }
  virtual float DayWaterTemperature() const { return m_dayWaterTemperature; }
  virtual void NightWaterTemperature(float value) { m_nightWaterTemperature = value; }
  virtual float NightWaterTemperature() const { return m_nightWaterTemperature; }
  virtual void DaySoilTemperature(float value) { m_daySoilTemperature = value; }
  virtual float DaySoilTemperature() const { return m_daySoilTemperature; }
  virtual void NightSoilTemperature(float value) { m_nightSoilTemperature = value; }
  virtual float NightSoilTemperature() const { return m_nightSoilTemperature; }
  virtual void DayAirTemperature(float value) { m_dayAirTemperature = value; }
  virtual float DayAirTemperature() const { return m_dayAirTemperature; }
  virtual void NightAirTemperature(float value) { m_nightAirTemperature = value; }
  virtual float NightAirTemperature() const { return m_nightAirTemperature; }
  virtual void WaterHeatingIsOn(bool value) { m_waterHeatingIsOn = value; }
  virtual bool WaterHeatingIsOn() const { return m_waterHeatingIsOn; }
  virtual void SoilHeatingIsOn(bool value) { m_soilHeatingIsOn = value; }
  virtual bool SoilHeatingIsOn() const { return m_soilHeatingIsOn; }
  virtual void AirHeatingIsOn(bool value) { m_airHeatingIsOn = value; }
  virtual bool AirHeatingIsOn() const { return m_airHeatingIsOn; }
  
private:
  void UpdateDayWaterHeating();
  void UpdateNightWaterHeating();

private:
  ::Log m_log;
  bool m_sensorWarningSent;
  bool m_autoMode;
  float m_openStart;
  float m_openFinish;
  int m_windowProgress;
  bool m_testMode;
  int m_openDayMinimum;
  float m_insideHumidityWarning;
  float m_soilMostureWarning;
  int m_dayStartHour;
  int m_dayEndHour;
  int m_windowActuatorSpeedPercent;
  float m_windowActuatorRuntimeSec;
  float m_dayWaterTemperature;
  float m_nightWaterTemperature;
  float m_daySoilTemperature;
  float m_nightSoilTemperature;
  float m_dayAirTemperature;
  float m_nightAirTemperature;
  bool m_waterHeatingIsOn;
  bool m_soilHeatingIsOn;
  bool m_airHeatingIsOn;
};

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
