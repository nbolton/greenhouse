#include "System.h"

#include <cstdlib>
#include <stdio.h>
#include <time.h>

namespace native {
namespace greenhouse {

const float k_windowAdjustThreshold = 0.05;
const float k_soilSensorDry = 3.93; // V, in air
const float k_soilSensorWet = 1.89; // V, in water
const int k_windowActuatorSpeedMax = 255;
const int k_dryWeatherCode = 701; // anything less is snow/rain

System::System() :
  m_log(),
  m_sensorWarningSent(false),
  m_autoMode(false),
  m_openStart(k_unknown),
  m_openFinish(k_unknown),
  m_windowProgress(k_unknown),
  m_testMode(false),
  m_insideHumidityWarning(k_unknown),
  m_soilMostureWarning(k_unknown),
  m_dayStartHour(k_unknown),
  m_dayEndHour(k_unknown),
  m_windowActuatorSpeedPercent(0),
  m_windowActuatorRuntimeSec(0),
  m_weatherCode(k_unknown),
  m_weatherInfo(),
  m_isRaining(false),
  m_systemStarted(false),
  m_weatherErrors(0),
  m_weatherErrorReportSent(false),
  m_dayNightTransitionTime(k_unknownUL)
{
}

void System::Setup() { Heating().System(*this); }

void System::Loop() {}

bool System::Refresh()
{
  Log().Trace("Refreshing");

  CheckDayNightTransition();

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
    "Moisture, inside=%.2f%%, outside=%.2f%%, soil=%.2f%%",
    InsideAirHumidity(),
    OutsideAirHumidity(),
    SoilMoisture());

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

  return sensorsOk;
}

bool System::ApplyWindowProgress(float expectedProgress)
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
      "Testing window progress, expected=%.2f, current=%.2f, open=%.2f, close=%.2f",
      expectedProgress,
      currentProgress,
      openDelta,
      closeDelta);

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
  int speed = m_windowActuatorSpeedPercent * ((float)k_windowActuatorSpeedMax / 100);
  Log().Trace("Actuator speed: %dms", speed);
  SetWindowActuatorSpeed(speed);

  RunWindowActuator(open);

  int runtime = (m_windowActuatorRuntimeSec * 1000) * delta;
  Log().Trace("Actuator runtime: %dms", runtime);
  SystemDelay(runtime);

  StopActuator();
}

float System::CalculateMoisture(float analogValue) const
{
  float percent = mapFloat(analogValue, k_soilSensorDry, k_soilSensorWet, 0, 100);
  Log().Trace("Soil moisture, analog=%.4fV, percent=%.2f%%", analogValue, percent);
  if (percent < -10 || percent > 110) {
    Log().Trace("Invalid soil moisture value");
    return k_unknown;
  }
  return percent;
}

bool System::IsRaining() const { return WeatherCode() < k_dryWeatherCode; }

void System::HandleNightDayTransition()
{
  m_weatherErrorReportSent = false;
  Heating().HandleDayNightTransition();
}

bool System::IsDaytime() const
{
  return (CurrentHour() >= DayStartHour()) && (CurrentHour() < DayEndHour());
}

void System::CheckDayNightTransition()
{
  // if time unknown, we can't do anything.
  if (EpochTime() == k_unknownUL) {
    Log().Trace("Epoch unknown, can't check day night transition");
    return;
  }

  time_t now = EpochTime();
  time_t last = DayNightTransitionTime();

  // HACK: cast to int to print -1 (%lld doesn't seem to print -1).
  // this is a bad idea, and will break in 2038.
  Log().Trace(
    "Checking day night transition, now=%d, last=%d",
    static_cast<int>(now),
    static_cast<int>(last));

  // only check transition time if there is one.
  if (last != k_unknownUL) {

    struct tm *ptm;

    ptm = gmtime(&now);
    int nowDay = ptm->tm_mday;
    int nowMonth = ptm->tm_mon;
    int nowYear = ptm->tm_year;

    ptm = gmtime(&last);
    int lastDay = ptm->tm_mday;
    int lastMonth = ptm->tm_mon;
    int lastYear = ptm->tm_year;

    Log().Trace("Epoch (now), day=%d, month=%d, year=%d", nowDay, nowMonth, nowYear);
    Log().Trace("Last transition, day=%d, month=%d, year=%d", lastDay, lastMonth, lastYear);

    // if days match, we already transitioned today.
    if ((nowDay == lastDay) && (nowMonth == lastMonth) && (nowYear == lastYear)) {
      Log().Trace("Day night transition already happened today");
      return;
    }
  }

  // wait until the start of the day to transition.
  if (IsDaytime()) {
    Log().Trace("Night/day transition detected");
    DayNightTransitionTime(EpochTime());
    HandleNightDayTransition();
  }
}

} // namespace greenhouse
} // namespace native