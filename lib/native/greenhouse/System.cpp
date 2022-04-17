#include "System.h"

#include <cstdlib>
#include <stdio.h>

namespace native {
namespace greenhouse {

const int k_dryWeatherCode = 701; // anything less is snow/rain
const int k_moistureMargin = 20;
const int k_soilMoistureSampleMax = 10;

System::System() :
  m_log(),
  m_sensorWarningSent(false),
  m_autoMode(false),
  m_openStart(k_unknown),
  m_openFinish(k_unknown),
  m_windowProgress(k_unknown),
  m_testMode(false),
  m_soilMostureWarning(k_unknown),
  m_windowActuatorRuntimeSec(0),
  m_weatherCode(k_unknown),
  m_weatherInfo(),
  m_isRaining(false),
  m_systemStarted(false),
  m_weatherErrors(0),
  m_weatherErrorReportSent(false),
  m_soilSensorWet(k_unknown),
  m_soilSensorDry(k_unknown),
  m_soilMoistureSampleMax(k_soilMoistureSampleMax),
  m_soilMoistureAverage(0)
{
}

void System::Setup()
{
  Time().System(*this);
  Heating().System(*this);
}

void System::Loop() {}

void System::Refresh()
{
  Log().Trace("Refreshing (native)");

  Time().CheckTransition();

  int sensorFailures = 0;
  bool sensorsOk = ReadSensors(sensorFailures);

  if (!sensorsOk) {
    // only send once per reboot (don't spam the timeline)
    if (!m_sensorWarningSent && SystemStarted()) {
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
    "Moisture, inside=%.2f%%, outside=%.2f%%, soil-now=%.2f%%, soil-avg=%.2f%%",
    InsideAirHumidity(),
    OutsideAirHumidity(),
    SoilMoisture(),
    SoilMoistureAverage());

  ReportSensorValues();
  ReportWarnings();

  // running weather report on startup seems to crash randomly,
  // so only check weather after start
  if (SystemStarted()) {
    const int weatherErrorReportAtCount = 4;
    if (!UpdateWeatherForecast()) {
      if ((++m_weatherErrors >= weatherErrorReportAtCount) && !m_weatherErrorReportSent) {
        ReportWarning("Weather update failed");
        m_weatherErrorReportSent = true;
      }
    }
    else {
      m_weatherErrors = 0;
      m_weatherErrorReportSent = false;
    }
  }

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

  Heating().Update();

  // Window state is only reported back when a window opens or closes;
  // Blynk doesn't show the latest switch values that were changed while the
  // app was out of focus, so if window isn't opened or closed, report the
  // current window state back.
  if (!windowMoved) {
    ReportWindowProgress();
  }

  ReportSystemInfo();

  Log().Trace("Refresh done (native)");
}

bool System::ApplyWindowProgress(float expectedProgress)
{
  // TODO: would 0/100 cause a random crash?
  float currentProgress = (float)WindowProgress() / 100;

  // avoid constant micro movements; only open/close window if difference is more than threshold
  float threshold = (float)m_windowAdjustThreshold / 100;
  bool overThreshold = std::abs(expectedProgress - currentProgress) > threshold;
  bool fullValue = (expectedProgress == 0) || (expectedProgress == 1);

  Log().Trace(
    "Apply window progress, threshold=%.2f, expected=%.2f, current=%.2f, over=%s, full=%s",
    threshold,
    expectedProgress,
    currentProgress,
    overThreshold ? "true" : "false",
    fullValue ? "true" : "false");

  if (overThreshold || fullValue) {

    float closeDelta = currentProgress - expectedProgress;
    float openDelta = expectedProgress - currentProgress;

    Log().Trace("Testing window progress, open=%.2f, close=%.2f", openDelta, closeDelta);

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

void System::AddWindowProgressDelta(float delta)
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

void System::OpenWindow(float delta)
{
  Log().Trace("Opening window...");
  Log().Trace("Delta: %.2f", delta);

  AddWindowProgressDelta(delta);
  ReportWindowProgress();
  AdjustWindow(true, delta);

  float percent = delta * 100;
  Log().Trace("Window opened %.1f%%", percent);
}

void System::CloseWindow(float delta)
{
  Log().Trace("Closing window...");
  Log().Trace("Delta: %.2f", delta);

  AddWindowProgressDelta(delta * -1);
  ReportWindowProgress();
  AdjustWindow(false, delta);

  float percent = delta * 100;
  Log().Trace("Window closed %.1f%%", percent);
}

void System::AdjustWindow(bool open, float delta)
{
  RunWindowActuator(open);

  int runtime = (m_windowActuatorRuntimeSec * 1000) * delta;
  Log().Trace("Actuator runtime: %dms", runtime);
  Delay(runtime);

  StopActuator();
}

float System::CalculateMoisture(float analogValue) const
{
  float percent = mapFloat(analogValue, SoilSensorDry(), SoilSensorWet(), 0, 100);
  Log().Trace("Soil moisture, analog=%.4fV, percent=%.2f%%", analogValue, percent);
  if (percent < (k_moistureMargin * -1) || percent > (100 + k_moistureMargin)) {
    Log().Trace("Invalid soil moisture value");
    return k_unknown;
  }
  return percent;
}

bool System::IsRaining() const { return WeatherCode() < k_dryWeatherCode; }

void System::HandleNightToDayTransition()
{
  m_weatherErrorReportSent = false;
  Heating().HandleNightToDayTransition();
}

void System::HandleDayToNightTransition() { Heating().HandleDayToNightTransition(); }

void System::SoilCalibrateWet()
{
  if (!ReadSoilMoistureSensor()) {
    Log().Trace("Unable to calibrate, failed to read soil moisture sensor");
    return;
  }

  Log().Trace("Calibrating soil moisture wet: %.2fV", SoilSensor());
  SoilSensorWet(SoilSensor());
  ReportMoistureCalibration();
  Refresh();
}

void System::SoilCalibrateDry()
{
  if (!ReadSoilMoistureSensor()) {
    Log().Trace("Unable to calibrate, failed to read soil moisture sensor");
    return;
  }

  Log().Trace("Calibrating soil moisture dry: %.2fV", SoilSensor());
  SoilSensorDry(SoilSensor());
  ReportMoistureCalibration();
  Refresh();
}

void System::AddSoilMoistureSample(float sample)
{
  m_soilMoistureSamples.push(sample);
  if (m_soilMoistureSamples.size() > SoilMoistureSampleMax()) {
    m_soilMoistureSamples.pop();
    Log().Trace("Moisture samples reduced by 1, new size: %d", m_soilMoistureSamples.size());
  }

  // we could use deque but this is fine for a short float list that isn't
  // accessed very often.
  float total = 0;
  const int size = m_soilMoistureSamples.size();
  std::queue<float> temp = m_soilMoistureSamples;
  while (!temp.empty()) {
    total += temp.front();
    temp.pop();
  }
  m_soilMoistureAverage = total / size;

  Log().Trace(
    "Calculating moisture average, total=%.0f, size=%d, avg=%.2f",
    total,
    size,
    m_soilMoistureAverage);
}

float System::SoilMoistureAverage() { return m_soilMoistureAverage; }

} // namespace greenhouse
} // namespace native