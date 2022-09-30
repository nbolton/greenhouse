#include "System.h"

#include <cstdlib>
#include <math.h>
#include <stdio.h>

namespace native {
namespace greenhouse {

const int k_dryWeatherCode = 701; // anything less is snow/rain
const int k_moistureMargin = 20;
const int k_soilMoistureSampleMax = 10;
const int k_windowActuatorLoopDelay = 100;

System::System() :
  m_sensorWarningSent(false),
  m_autoMode(false),
  m_openStart(k_unknown),
  m_openFinish(k_unknown),
  m_windowProgressExpected(k_unknown),
  m_windowProgressActual(k_unknown),
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
  m_soilMoistureAverage(0),
  m_windowAdjustPositions(k_unknown),
  m_windowAdjustTimeframe(k_unknown),
  m_windowAdjustLast(k_unknownUL),
  m_windowActuatorStopTime(k_unknownUL)
{
}

void System::Setup()
{
  Time().System(*this);
  Heating().System(*this);
}

void System::Loop()
{
  if (IsWindowActuatorRunning()) {
    const int windowTimeLeft = m_windowActuatorStopTime - Time().EpochTime();
    if (windowTimeLeft <= 0) {
      TRACE_F("Actuator finished, overshoot: %ds", abs(windowTimeLeft));
      m_windowActuatorStopTime = k_unknownUL;
      StopActuator();
    }
    else {
      TRACE_F("Actuator opening window, time left: %ds", windowTimeLeft);
      Delay(k_windowActuatorLoopDelay, "Actuator loop");
      return;
    }
  }
}

void System::Refresh()
{
  TRACE("Refreshing (native)");

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
      TRACE_F("Sensors unavailable, failures: %d", sensorFailures);
    }
  }

  TRACE_F(
    "Temperatures, inside=%.2f°C, outside=%.2f°C, soil=%.2f°C, water=%.2f°C",
    InsideAirTemperature(),
    OutsideAirTemperature(),
    SoilTemperature(),
    WaterTemperature());

  TRACE_F(
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
    if (WindowProgressExpected() > 0) {
      // close windows on rain even if in manual mode (it may get left on by accident)
      ReportInfo("Weather forecast is rain, closing window");
      CloseWindow((float)WindowProgressExpected() / 100);
    }
  }
  else if (m_autoMode) {
    float soilTemperature = SoilTemperature();

    TRACE_F(
      "Auto mode, wp=%d%% t=%.2fC os=%.2fC of=%.2fC last=%d timeframe=%dm",
      WindowProgressExpected(),
      soilTemperature,
      openStart,
      openFinish,
      (int)m_windowAdjustLast, // HACK
      m_windowAdjustTimeframe);

    int timeframeSec = m_windowAdjustTimeframe * 60;
    bool timeframeOk = (m_windowAdjustTimeframe == k_unknown) ||
                       (m_windowAdjustLast == k_unknownUL) ||
                       ((int)(Time().EpochTime() - m_windowAdjustLast) > timeframeSec);
    bool noUnknowns =
      (soilTemperature != k_unknown) && (m_openStart != k_unknown) && (m_openFinish != k_unknown);

    if (noUnknowns && timeframeOk) {

      if ((soilTemperature > openStart) && (soilTemperature < openFinish)) {
        // window should be semi-open
        TRACE("Temperature in bounds");

        float tempWidth = openFinish - openStart;
        float progressAsTemp = soilTemperature - openStart;
        m_windowProgressExpected = (progressAsTemp / tempWidth) * 100;
      }
      else if (soilTemperature >= openFinish) {
        // window should be fully open
        TRACE("Temperature above bounds");
        m_windowProgressExpected = 100;
      }
      else {
        // window should be fully closed
        TRACE("Temperature below bounds");
        m_windowProgressExpected = 0;
      }

      windowMoved = ApplyWindowProgress();
    }
    else {
      TRACE_F(
        "Window adjust didn't run, noUnknowns=%s, timeframeOk=%s",
        noUnknowns ? "true" : "false",
        timeframeOk ? "true" : "false");
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

  TRACE("Refresh done (native)");
}

bool System::ApplyWindowProgress()
{
  const float positions = WindowAdjustPositions();
  const float expected = WindowProgressExpected();
  const float actual = WindowProgressActual();

  bool fullExtent = (expected == 0) || (expected == 1);
  float rounded = (round((positions / 100) * expected) / positions) * 100;
  bool changed = std::abs(rounded - actual) > 0.01f;

  TRACE_F(
    "Apply window progress, "
    "positions=%d, expected=%.2f, rounded=%.2f, actual=%.2f, "
    "changed=%s, full=%s",
    (int)positions,
    expected,
    rounded,
    actual,
    changed ? "true" : "false",
    fullExtent ? "true" : "false");

  if (positions == k_unknown) {
    TRACE("Cannot apply window progress with unknown positions");
    return false;
  }

  if (changed || fullExtent) {

    float closeDelta = (actual - rounded) / 100;
    float openDelta = (rounded - actual) / 100;

    TRACE_F("Testing window progress, open=%.2f, close=%.2f", openDelta, closeDelta);

    if (openDelta > 0) {
      OpenWindow(openDelta);
      m_windowAdjustLast = Time().EpochTime();
      return true;
    }
    else if (closeDelta > 0) {
      CloseWindow(closeDelta);
      m_windowAdjustLast = Time().EpochTime();
      return true;
    }
  }

  TRACE("No window progress change");
  return false;
}

void System::AddWindowProgressActualDelta(float delta)
{
  int wp = m_windowProgressActual;
  wp += delta * 100;
  if (wp < 0) {
    wp = 0;
  }
  else if (wp > 100) {
    wp = 100;
  }
  m_windowProgressActual = wp;
}

void System::OpenWindow(float delta)
{
  TRACE("Opening window...");
  TRACE_F("Delta: %.2f", delta);

  AddWindowProgressActualDelta(delta);
  ReportWindowProgress();
  AdjustWindow(true, delta);

  float percent = delta * 100;
  TRACE_F("Window opened %.1f%%", percent);
}

void System::CloseWindow(float delta)
{
  TRACE("Closing window...");
  TRACE_F("Delta: %.2f", delta);

  AddWindowProgressActualDelta(delta * -1);
  ReportWindowProgress();
  AdjustWindow(false, delta);

  float percent = delta * 100;
  TRACE_F("Window closed %.1f%%", percent);
}

void System::AdjustWindow(bool open, float delta)
{
  RunWindowActuator(open);

  int runtime = m_windowActuatorRuntimeSec * delta;
  TRACE_F("Actuator runtime set: %ds", runtime);

  m_windowActuatorStopTime = Time().EpochTime() + runtime;
}

float System::CalculateMoisture(float analogValue) const
{
  float percent = mapFloat(analogValue, SoilSensorDry(), SoilSensorWet(), 0, 100);
  TRACE_F("Soil moisture, analog=%.4fV, percent=%.2f%%", analogValue, percent);
  if (percent < (k_moistureMargin * -1) || percent > (100 + k_moistureMargin)) {
    TRACE("Invalid soil moisture value");
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

void System::ResetSoilMoistureAverage()
{
  while (!m_soilMoistureSamples.empty())
  {
    m_soilMoistureSamples.pop();
  }
}

void System::SoilCalibrateWet()
{
  if (!ReadSoilMoistureSensor()) {
    TRACE("Unable to calibrate, failed to read soil moisture sensor");
    return;
  }

  TRACE_F("Calibrating soil moisture wet: %.2fV", SoilSensor());
  SoilSensorWet(SoilSensor());
  ReportMoistureCalibration();
  ResetSoilMoistureAverage();
  Refresh();
}

void System::SoilCalibrateDry()
{
  if (!ReadSoilMoistureSensor()) {
    TRACE("Unable to calibrate, failed to read soil moisture sensor");
    return;
  }

  TRACE_F("Calibrating soil moisture dry: %.2fV", SoilSensor());
  SoilSensorDry(SoilSensor());
  ReportMoistureCalibration();
  ResetSoilMoistureAverage();
  Refresh();
}

void System::AddSoilMoistureSample(float sample)
{
  m_soilMoistureSamples.push(sample);
  if (m_soilMoistureSamples.size() > SoilMoistureSampleMax()) {
    m_soilMoistureSamples.pop();
    TRACE_F("Moisture samples reduced by 1, new size: %d", m_soilMoistureSamples.size());
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

  TRACE_F(
    "Calculating moisture average, total=%.0f, size=%d, avg=%.2f",
    total,
    size,
    m_soilMoistureAverage);
}

float System::SoilMoistureAverage() { return m_soilMoistureAverage; }

void System::UpdateWindowProgress()
{
  TRACE_F(
    "Window progress new=%d, current=%d, actual=%d",
    m_windowProgress,
    m_windowProgressExpected,
    m_windowProgressActual);

  int firstTime = m_windowProgressExpected == k_unknown;
  m_windowProgressExpected = m_windowProgress;

  if (firstTime) {
    m_windowProgressActual = m_windowProgressExpected;
  }
  else {
    // only apply window progress if it's not the 1st time;
    // otherwise the window will always open from 0 on start,
    // and the position might be something else.
    ApplyWindowProgress();
  }
}

} // namespace greenhouse
} // namespace native