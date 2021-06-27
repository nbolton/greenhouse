#include "Greenhouse.h"

#include <cstdlib>
#include <stdio.h>

const float k_windowAdjustThreshold = 0.05;
const int k_dayStartHour = 8; // 8am
const int k_dayEndHour = 20;  // 8pm
const int k_analogMoistureMin = 100;
const int k_analogMoistureMax = 900;

Greenhouse::Greenhouse() :
  m_sensorWarningSent(false),
  m_autoMode(false),
  m_openStart(k_unknown),
  m_openFinish(k_unknown),
  m_windowProgress(k_unknown),
  m_openDayMinimum(k_unknown),
  m_insideHumidityWarning(k_unknown),
  m_soilMostureWarning(k_unknown)
{
}

void Greenhouse::Setup() {}

void Greenhouse::Loop() {}

bool Greenhouse::Refresh()
{
  Log().Trace("Refreshing");

  bool sensorsOk = ReadSensors();
  float insideTemperature = InsideTemperature();
  float outsideTemperature = OutsideTemperature();
  float soilTemperature = SoilTemperature();

  if (!sensorsOk) {
    Log().Trace("Sensors unavailable");

    // only send once per reboot (don't spam the timeline)
    if (!m_sensorWarningSent) {
      ReportWarning("Sensors unavailable");
      m_sensorWarningSent = true;
    }
  }

  Log().Trace(
    "Temperatures, inside=%.2fC, outside=%.2f%%, soil=%.2f%%",
    insideTemperature,
    outsideTemperature,
    soilTemperature);

  ReportSensorValues();

  ReportWarnings();

  FlashLed(k_ledRefresh);

  bool windowMoved = false;

  // TODO: https://github.com/nbolton/home-automation/issues/20
  float openStart = (float)m_openStart;
  float openFinish = (float)m_openFinish;

  if (m_autoMode) {
    Log().Trace(
      "Auto mode, wp=%d%% t=%.2fC os=%.2fC of=%.2fC",
      WindowProgress(),
      soilTemperature,
      openStart,
      openFinish);

    if (
      (soilTemperature != k_unknown) && (m_openStart != k_unknown) &&
      (m_openFinish != k_unknown)) {

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

    bool isDayPeriod = (CurrentHour() >= k_dayStartHour) && (CurrentHour() <= k_dayEndHour);

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
  AddWindowProgressDelta(delta);
  ReportWindowProgress();
}

void Greenhouse::CloseWindow(float delta)
{
  AddWindowProgressDelta(delta * -1);
  ReportWindowProgress();
}

float Greenhouse::CalculateMoisture(int analogValue) const
{
  if (analogValue <= k_analogMoistureMin) {
    return 0;
  }

  if (analogValue >= k_analogMoistureMax) {
    return 100;
  }

  int range = k_analogMoistureMax - k_analogMoistureMin;
  return ((float)(analogValue - k_analogMoistureMin) / range) * 100;
}
