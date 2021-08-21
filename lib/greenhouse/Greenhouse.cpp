#include "Greenhouse.h"

#include <cstdlib>
#include <stdio.h>

const float k_windowAdjustThreshold = 0.05;
const float k_soilSensorDry = 3.95; // V, in air
const float k_soilSensorWet = 1.9;  // V, in water
const int k_windowActuatorSpeedMax = 255;
const float k_waterTempMargin = 1;
const float k_soilTempMargin = .2f;
const float k_airTempMargin = 1;
const int k_dryWeatherCode = 701; // anything less is snow/rain

Greenhouse::Greenhouse() :
  m_sensorWarningSent(false),
  m_autoMode(false),
  m_openStart(k_unknown),
  m_openFinish(k_unknown),
  m_windowProgress(k_unknown),
  m_testMode(false),
  m_openDayMinimum(k_unknown),
  m_insideHumidityWarning(k_unknown),
  m_soilMostureWarning(k_unknown),
  m_dayStartHour(k_unknown),
  m_dayEndHour(k_unknown),
  m_windowActuatorSpeedPercent(0),
  m_windowActuatorRuntimeSec(0),
  m_dayWaterTemperature(k_unknown),
  m_nightWaterTemperature(k_unknown),
  m_daySoilTemperature(k_unknown),
  m_nightSoilTemperature(k_unknown),
  m_dayAirTemperature(k_unknown),
  m_nightAirTemperature(k_unknown),
  m_waterHeatingIsOn(false),
  m_soilHeatingIsOn(false),
  m_airHeatingIsOn(false),
  m_weatherCode(k_unknown),
  m_weatherInfo(),
  m_isRaining(false),
  m_waterHeaterLimitMinutes(k_unknown),
  m_waterHeatingStartSeconds(k_unknownUL),
  m_waterHeatingRuntimeSeconds(0),
  m_waterHeatingWasDaytime(false),
  m_waterHeatingHasRun(false)
{
}

void Greenhouse::Setup() {}

void Greenhouse::Loop() {}

bool Greenhouse::Refresh()
{
  Log().Trace("Refreshing");

  int sensorFailures = 0;
  bool sensorsOk = ReadSensors(sensorFailures);

  if (!sensorsOk) {
    // only send once per reboot (don't spam the timeline)
    if (!m_sensorWarningSent) {
      ReportWarning("Sensors unavailable, failures: %d", sensorFailures);
      m_sensorWarningSent = true;
    }
    else {
      Log().Trace("Sensors unavailable, failures: %d", sensorFailures);
    }
  }

  Log().Trace(
    "Temperatures, inside=%.2f°C, outside=%.2f°C, soil=%.2f°C, water=%.2f°C",
    InsideAirTemperature(),
    OutsideAirTemperature(),
    SoilTemperature(),
    WaterTemperature());

  Log().Trace(
    "Moisture, inside=%.2f%%, outside=%.2f%%, soil=%.2f%%",
    InsideAirHumidity(),
    OutsideAirHumidity(),
    SoilMoisture());

  ReportSensorValues();

  ReportWarnings();

  bool windowMoved = false;
  float openStart = m_openStart;
  float openFinish = m_openFinish;

  if ((WeatherCode() != k_unknown) && IsRaining()) {
    if (WindowProgress() > 0) {
      // close windows on rain even if in manual mode (it may get left on by accident)
      ReportInfo("Weather forecast is rain, closing window");
      CloseWindow((float)WindowProgress() / 100);
    }
  }
  else if (m_autoMode) {
    float soilTemperature = SoilTemperature();

    Log().Trace(
      "Auto mode, wp=%d%% t=%.2fC os=%.2fC of=%.2fC",
      WindowProgress(),
      soilTemperature,
      openStart,
      openFinish);

    if (
      (soilTemperature != k_unknown) && (m_openStart != k_unknown) && (m_openFinish != k_unknown)) {

      float expectedProgress = 0;

      if ((soilTemperature > openStart) && (soilTemperature < openFinish)) {
        // window should be semi-open
        Log().Trace("Temperature in bounds");

        float tempWidth = openFinish - openStart;
        float progressAsTemp = soilTemperature - openStart;
        expectedProgress = progressAsTemp / tempWidth;
      }
      else if (soilTemperature >= openFinish) {
        // window should be fully open
        Log().Trace("Temperature above bounds");
        expectedProgress = 1;
      }
      else {
        // window should be fully closed
        Log().Trace("Temperature below bounds");
      }

      windowMoved = ApplyWindowProgress(expectedProgress);
    }
  }

  UpdateHeatingSystems();

  // Window state is only reported back when a window opens or closes;
  // Blynk doesn't show the latest switch values that were changed while the
  // app was out of focus, so if window isn't opened or closed, report the
  // current window state back.
  if (!windowMoved) {
    ReportWindowProgress();
  }

  ReportSystemInfo();

  return sensorsOk;
}

bool Greenhouse::ApplyWindowProgress(float expectedProgress)
{
  // TODO: would 0/100 cause a random crash?
  float currentProgress = (float)WindowProgress() / 100;

  // avoid constant micro movements; only open/close window if difference is more than threshold
  bool overThreshold = std::abs(expectedProgress - currentProgress) > k_windowAdjustThreshold;
  bool fullValue = (expectedProgress == 0) || (expectedProgress == 1);

  if (overThreshold || fullValue) {

    float closeDelta = currentProgress - expectedProgress;
    float openDelta = expectedProgress - currentProgress;

    Log().Trace(
      "Testing window progress, expected=%.2f, current=%.2f, open=%.2f, close=%.2f, min=%d%%",
      expectedProgress,
      currentProgress,
      openDelta,
      closeDelta,
      OpenDayMinimum());

    bool isDayPeriod = (CurrentHour() >= DayStartHour()) && (CurrentHour() <= DayEndHour());

    // if open day minimum is set and it's day:
    // - don't allow window to be closed less than open day minimum.
    // - if already closed beyond minimum, open it up to match minimum.
    if (isDayPeriod && (OpenDayMinimum() > 0)) {

      float min = (float)OpenDayMinimum() / 100;
      closeDelta -= min;
      openDelta += min;

      Log().Trace("Deltas adjusted, open=%.2f, close=%.2f", openDelta, closeDelta);
    }

    if (openDelta > 0) {
      OpenWindow(openDelta);
      return true;
    }
    else if (closeDelta > 0) {
      CloseWindow(closeDelta);
      return true;
    }
  }

  Log().Trace("No window progress change");
  return false;
}

void Greenhouse::AddWindowProgressDelta(float delta)
{
  int wp = WindowProgress();
  wp += delta * 100;
  if (wp < 0) {
    wp = 0;
  }
  else if (wp > 100) {
    wp = 100;
  }
  WindowProgress(wp);
}

void Greenhouse::OpenWindow(float delta)
{
  Log().Trace("Opening window...");
  Log().Trace("Delta: %.2f", delta);

  AddWindowProgressDelta(delta);
  ReportWindowProgress();
  AdjustWindow(true, delta);

  float percent = delta * 100;
  Log().Trace("Window opened %.1f%%", percent);
}

void Greenhouse::CloseWindow(float delta)
{
  Log().Trace("Closing window...");
  Log().Trace("Delta: %.2f", delta);

  AddWindowProgressDelta(delta * -1);
  ReportWindowProgress();
  AdjustWindow(false, delta);

  float percent = delta * 100;
  Log().Trace("Window closed %.1f%%", percent);
}

void Greenhouse::AdjustWindow(bool open, float delta)
{
  int speed = m_windowActuatorSpeedPercent * ((float)k_windowActuatorSpeedMax / 100);
  Log().Trace("Actuator speed: %dms", speed);
  SetWindowActuatorSpeed(speed);

  RunWindowActuator(open);

  int runtime = (m_windowActuatorRuntimeSec * 1000) * delta;
  Log().Trace("Actuator runtime: %dms", runtime);
  SystemDelay(runtime);

  StopActuator();
}

// float version of Arduino map()
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / ((in_max - in_min) + out_min);
}

float Greenhouse::CalculateMoisture(float analogValue) const
{
  float percent = mapFloat(analogValue, k_soilSensorDry, k_soilSensorWet, 0, 100);
  Log().Trace("Soil moisture, analog=%.2fV, percent=%.2f%%", analogValue, percent);
  if (percent < -10 || percent > 110) {
    Log().Trace("Invalid soil moisture value");
    return k_unknown;
  }
  return percent;
}

void Greenhouse::SwitchWaterHeatingIfChanged(bool on)
{
  if (on && 
    (m_waterHeatingRuntimeSeconds > 0) &&
    ((int)(m_waterHeatingRuntimeSeconds / 60) >= m_waterHeaterLimitMinutes)) {

    Log().Trace("Blocking water heating switch on, runtime=%ds limit=%dm", 
      m_waterHeatingRuntimeSeconds, m_waterHeaterLimitMinutes);
    return;
  }

  if (WaterHeatingIsOn() != on) {
    WaterHeatingIsOn(on);
    SwitchWaterHeating(on);
    m_waterHeatingStartSeconds = UptimeSeconds();
  }
}

void Greenhouse::SwitchSoilHeatingIfChanged(bool on)
{
  if (SoilHeatingIsOn() != on) {
    SoilHeatingIsOn(on);
    SwitchSoilHeating(on);
  }
}

void Greenhouse::SwitchAirHeatingIfChanged(bool on)
{
  if (AirHeatingIsOn() != on) {
    AirHeatingIsOn(on);
    SwitchAirHeating(on);
  }
}

void Greenhouse::UpdateDayWaterHeating()
{
  if (WaterTemperature() < (DayWaterTemperature() - k_waterTempMargin)) {

    if (AirHeatingIsOn() || SoilHeatingIsOn()) {
      // only switch water heating on if needed
      SwitchWaterHeatingIfChanged(true);
    }
    else {
      // otherwise, turn water heating off (not needed)
      SwitchWaterHeatingIfChanged(false);
    }
  }
  else if (WaterTemperature() > (DayWaterTemperature() + k_waterTempMargin)) {
    
    // always switch off, even if soil/air heating on
    SwitchWaterHeatingIfChanged(false);
  }
}

void Greenhouse::UpdateNightWaterHeating()
{
  if (WaterTemperature() < (NightWaterTemperature() - k_waterTempMargin)) {
    
    // only switch water heating on if needed
    if (AirHeatingIsOn() || SoilHeatingIsOn()) {
      SwitchWaterHeatingIfChanged(true);
    }
  }
  else if (WaterTemperature() > (NightWaterTemperature() + k_waterTempMargin)) {

    // always switch water off if above limit, even if soil/air heating on
    SwitchWaterHeatingIfChanged(false);
  }
}

void Greenhouse::UpdateHeatingSystems()
{
  Log().Trace("Update heating systems, hour=%d", CurrentHour());

  if (WaterHeatingIsOn() && (m_waterHeatingStartSeconds != k_unknownUL)) {
    
    int period = (UptimeSeconds() - m_waterHeatingStartSeconds);
    m_waterHeatingRuntimeSeconds += period;
    ReportWaterHeatingRuntime();
    Log().Trace("Advanced water heating runtime, period=%ds, total=%ds", period, m_waterHeatingRuntimeSeconds);

    if ((int)(m_waterHeatingRuntimeSeconds / 60) >= m_waterHeaterLimitMinutes) {
      ReportInfo("Water heater runtime limit reached (%dm), switching off", m_waterHeaterLimitMinutes);
      SwitchWaterHeatingIfChanged(false);
    }
  }

  // heat water to different temperature depending on if day or night
  if ((CurrentHour() >= DayStartHour()) && (CurrentHour() < DayEndHour())) {

    // detect transition from night to day
    if (m_waterHeatingHasRun && !m_waterHeatingWasDaytime) {

      // reset runtime back to 0
      WaterHeatingRuntimeSeconds(0);
      ReportWaterHeatingRuntime();
      m_waterHeatingStartSeconds = k_unknownUL;
      Log().Trace("Water heater runtime reset");
    }

    Log().Trace("Daytime heating mode");
    m_waterHeatingWasDaytime = true;

    if (SoilTemperature() < (DaySoilTemperature() - k_soilTempMargin)) {

      Log().Trace("Day soil temp below");
      SwitchSoilHeatingIfChanged(true);
    }
    else if (SoilTemperature() > (DaySoilTemperature() + k_soilTempMargin)) {
      
      Log().Trace("Day soil temp above");
      SwitchSoilHeatingIfChanged(false);
    }

    if (InsideAirTemperature() < (DayAirTemperature() - k_airTempMargin)) {
      
      Log().Trace("Day air temp below");
      SwitchAirHeatingIfChanged(true);
    }
    else if (InsideAirTemperature() > (DayAirTemperature() + k_airTempMargin)) {
      
      Log().Trace("Day air temp above");
      SwitchAirHeatingIfChanged(false);
    }
    
    UpdateDayWaterHeating();
  }
  else if ((CurrentHour() < DayStartHour()) || (CurrentHour() >= DayEndHour())) {

    Log().Trace("Nighttime heating mode");
    m_waterHeatingWasDaytime = false;

    if (SoilTemperature() < (NightSoilTemperature() - k_soilTempMargin)) {
      
      Log().Trace("Night soil temp below");
      SwitchSoilHeatingIfChanged(true);
    }
    else if (SoilTemperature() > (NightSoilTemperature() + k_soilTempMargin)) {
      
      Log().Trace("Night soil temp above");
      SwitchSoilHeatingIfChanged(false);
    }

    if (InsideAirTemperature() < (NightAirTemperature() - k_airTempMargin)) {
      
      Log().Trace("Night air temp below");
      SwitchAirHeating(true);
    }
    else if (InsideAirTemperature() > (NightAirTemperature() + k_airTempMargin)) {
      
      Log().Trace("Night air temp above");
      SwitchAirHeating(false);
    }
    
    UpdateNightWaterHeating();
  }

  m_waterHeatingHasRun = true;
}

bool Greenhouse::IsRaining() const { return WeatherCode() < k_dryWeatherCode; }
