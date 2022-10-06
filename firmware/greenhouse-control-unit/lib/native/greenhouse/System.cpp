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
  m_applyWindowOpenPercent(false),
  m_windowOpenPercentExpected(k_unknown),
  m_windowOpenPercentActual(k_unknown),
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
  m_windowAdjustTimeframeSec(k_unknown),
  m_windowAdjustLast(k_unknownUL),
  m_windowActuatorStopTime(k_unknownUL),
  m_fakeWeatherCode(k_unknown)
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
  TRACE("Refreshing (native)");

  Time().CheckTransition();

  int sensorFailures = 0;
  const bool sensorsOk = ReadSensors(sensorFailures);

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

  const float openStart = m_openStart;
  const float openFinish = m_openFinish;

  if ((WeatherCode() != k_unknown) && IsRaining()) {
    if (WindowOpenPercentExpected() > 0) {
      // close windows on rain even if in manual mode (it may get left on by accident)
      ReportInfo("Weather forecast is rain (%d), closing window", WeatherCode());
      m_windowOpenPercentExpected = 0;
      ApplyWindowOpenPercent();
    }
  }
  else if (m_autoMode) {
    const float soilTemperature = SoilTemperature();

    TRACE_F(
      "Auto mode: expected=%d%%, actual=%d%%, soil=%.2fC, start=%.2fC, finish=%.2fC, last=%lu, "
      "timeframe=%ds",
      WindowOpenPercentExpected(),
      WindowOpenPercentActual(),
      soilTemperature,
      openStart,
      openFinish,
      m_windowAdjustLast,
      m_windowAdjustTimeframeSec);

    const int sinceLast = (int)(Time().UptimeSeconds() - m_windowAdjustLast);

    const bool timeframeOk = (m_windowAdjustTimeframeSec == k_unknown) ||
                             (m_windowAdjustLast == k_unknownUL) ||
                             (sinceLast > m_windowAdjustTimeframeSec);

    const bool noUnknowns =
      (soilTemperature != k_unknown) && (m_openStart != k_unknown) && (m_openFinish != k_unknown);

    if (noUnknowns && timeframeOk) {

      if ((soilTemperature > openStart) && (soilTemperature < openFinish)) {
        // window should be semi-open
        TRACE("Temperature in bounds");

        const float tempWidth = openFinish - openStart;
        const float openPercentAsTemp = soilTemperature - openStart;
        m_windowOpenPercentExpected = (openPercentAsTemp / tempWidth) * 100;
      }
      else if (soilTemperature >= openFinish) {
        // window should be fully open
        TRACE("Temperature above bounds");
        m_windowOpenPercentExpected = 100;
      }
      else {
        // window should be fully closed
        TRACE("Temperature below bounds");
        m_windowOpenPercentExpected = 0;
      }
      TRACE_F("Window open percent expected: %d%%", m_windowOpenPercentExpected);

      ApplyWindowOpenPercent();
    }
    else {
      TRACE_F(
        "Window adjust didn't run, noUnknowns=%s, timeframeOk=%s",
        noUnknowns ? "true" : "false",
        timeframeOk ? "true" : "false");
    }
  }

  Heating().Update();

  ReportSystemInfo();

  TRACE("Refresh done (native)");
}

void System::WindowFullClose()
{
  m_windowOpenPercentExpected = 0;
  CloseWindow(1);
  ReportWindowOpenPercent();
}

void System::ApplyWindowOpenPercent()
{
  const float changeThreshold = 0.02f;
  const float positions = WindowAdjustPositions();
  const float expected = WindowOpenPercentExpected();
  const float actual = WindowOpenPercentActual();

  const bool fullExtent = (expected == 0) || (expected == 1);
  const float rounded = (round((positions / 100) * expected) / positions) * 100;
  bool const changed = std::abs(expected - actual) > changeThreshold;

  TRACE_F(
    "Apply window open percent, "
    "positions=%d, expected=%.2f, rounded=%.2f, actual=%.2f, "
    "changed=%s, full=%s",
    (int)positions,
    expected,
    rounded,
    actual,
    changed ? "true" : "false",
    fullExtent ? "true" : "false");

  if (expected == k_unknown) {
    TRACE("Skip apply window open percent, unknown expected");
    return;
  }

  if (actual == k_unknown) {
    TRACE("Skip apply window open percent, unknown actual");
    return;
  }

  if (positions == k_unknown) {
    TRACE("Skip apply window open percent, unknown positions");
    return;
  }

  if (changed || fullExtent) {

    float closeDelta = (actual - rounded) / 100;
    float openDelta = (rounded - actual) / 100;

    TRACE_F("Testing window open percent, open=%.2f, close=%.2f", openDelta, closeDelta);

    if (openDelta > 0) {
      OpenWindow(openDelta);
      ReportWindowOpenPercent();
      m_windowAdjustLast = Time().UptimeSeconds();
      return;
    }
    else if (closeDelta > 0) {
      CloseWindow(closeDelta);
      ReportWindowOpenPercent();
      m_windowAdjustLast = Time().UptimeSeconds();
      return;
    }
  }

  TRACE("No window open percent change");
}

void System::ChangeWindowOpenPercentActual(float delta)
{
  int wp = m_windowOpenPercentActual;
  wp += delta * 100;
  if (wp < 0) {
    wp = 0;
  }
  else if (wp > 100) {
    wp = 100;
  }
  m_windowOpenPercentActual = wp;
}

void System::OpenWindow(float delta)
{
  TRACE_F("Opening window, delta=%.2f", delta);

  ChangeWindowOpenPercentActual(delta);
  RunWindowActuator(true, delta);

  float percent = delta * 100;
  TRACE_F("Window opened by %.1f%%", percent);
}

void System::CloseWindow(float delta)
{
  TRACE_F("Closing window, delta=%.2f", delta);

  ChangeWindowOpenPercentActual(delta * -1);
  RunWindowActuator(false, delta);

  float percent = delta * 100;
  TRACE_F("Window closed by %.1f%%", percent);
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

void System::WindowOpenPercent(int value)
{
  if (m_windowOpenPercentExpected == k_unknown) {
    // must be the first time we got the value from blynk.
    m_windowOpenPercentActual = value;
  }
  else {
    // if not the first time, apply the window open percent.
    // only apply window open percent if it's not the 1st time;
    // otherwise the window will always open from 0 on start,
    // and the position might be something else.
    m_applyWindowOpenPercent = true;
  }
  m_windowOpenPercentExpected = value;
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
  while (!m_soilMoistureSamples.empty()) {
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

void System::UpdateWindowOpenPercent()
{
  if (m_applyWindowOpenPercent) {
    ApplyWindowOpenPercent();
  }
}

int System::WeatherCode() const
{
  if (m_fakeWeatherCode != k_unknown) {
    return m_fakeWeatherCode;
  }
  return m_weatherCode;
}

} // namespace greenhouse
} // namespace native