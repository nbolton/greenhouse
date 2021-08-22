#pragma once

#include "Greenhouse.h"

#include "ArduinoLog.h"

#include <ADS1115_WE.h>
#include <Arduino.h>

const int k_switchCount = 4;

class Adafruit_SHT31;
struct ADC;

enum LedFlashTimes {
  k_ledRefresh = 1,
  k_ledSend = 2,
  k_ledRecieve = 3,
  k_ledStarted = 4,
  k_ledRestart = 5
};

class GreenhouseArduino : public Greenhouse {
public:
  static GreenhouseArduino &Instance();
  static void Instance(GreenhouseArduino &ga);

  GreenhouseArduino();

  void Setup();
  void Loop();
  bool Refresh();
  void RelayCallback();
  void WeatherCallback();
  void LastWrite();
  void SystemStarted();
  void Restart();
  void ToggleActiveSwitch();
  void HandleWindowProgress(int value);
  void ManualRefresh();

  const ArduinoLog &Log() const { return m_log; }

protected:
  void ReportInfo(const char *m, ...) const;
  void ReportWarning(const char *m, ...) const;
  void ReportCritical(const char *m, ...) const;
  void ReportSensorValues();
  void ReportWindowProgress();
  void ReportSystemInfo();
  void ReportWarnings();
  void ReportWeather();
  void ReportWaterHeatingInfo();

  void FlashLed(LedFlashTimes times);
  bool ReadSensors(int &failures);
  void SwitchWaterHeating(bool on);
  void SwitchSoilHeating(bool on);
  void SwitchAirHeating(bool on);
  void RunWindowActuator(bool forward);
  void StopActuator();
  void SetWindowActuatorSpeed(int speed);
  void SystemDelay(unsigned long ms);
  void UpdateWeatherForecast();

public:
  // getters & setters
  int CurrentHour() const;
  unsigned long UptimeSeconds() const;
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
  void PvForceOn(int value) { m_pvForceOn = value; }

private:
  void StartBeep(int times);
  void Actuator(bool forward, int s, int t);
  void SetSwitch(int i, bool on);
  void SwitchPower(bool pv);
  void MeasureVoltage();
  void MeasureCurrent();
  float ReadAdc(ADC &adc, ADS1115_MUX channel);
  void InitSensors();
  void InitADCs();
  void InitActuators();
  void InitShiftRegisters();
  void InitPowerSource();

private:
  ArduinoLog m_log;
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
  int m_pvVoltageCount;
  float m_pvVoltageAverage;
  float m_pvVoltageSum;
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
  bool m_pvForceOn;
};
