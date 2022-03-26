#pragma once

#include "../native/greenhouse/System.h"

using namespace native::greenhouse;

class TestSystem : public System {

public:
  TestSystem() :

    // calls
    m_calls_OpenWindow(0),
    m_calls_CloseWindow(0),
    m_calls_RunWindowActuator(0),
    m_calls_StopActuator(0),
    m_calls_SystemDelay(0),
    m_calls_ReportWarning(0),
    m_calls_HandleNightToDayTransition(0),
    m_calls_HandleDayToNightTransition(0),

    // mock returns
    m_mock_ReadDhtSensor(false),
    m_mock_WaterTemperature(0),
    m_mock_SoilTemperature(0),
    m_mock_InsideAirTemperature(0),
    m_mock_CurrentHour(0),
    m_mock_UptimeSeconds(0),
    m_mock_IsDaytime(false),
    m_mock_EpochTime(0),

    // mock enable
    m_mockOn_IsDaytime(false)
  {
    Heating().System(*this);
  }

  // mocks

  bool ReadSensors()
  {
    Log().Trace("Mock: ReadSensors, value=%s", m_mock_ReadDhtSensor ? "true" : "false");
    return m_mock_ReadDhtSensor;
  }

  float WaterTemperature() const
  {
    Log().Trace("Mock: WaterTemperature, value=%.2f", m_mock_WaterTemperature);
    return m_mock_WaterTemperature;
  }

  float SoilTemperature() const
  {
    Log().Trace("Mock: SoilTemperature, value=%.2f", m_mock_SoilTemperature);
    return m_mock_SoilTemperature;
  }

  float InsideAirTemperature() const
  {
    Log().Trace("Mock: InsideAirTemperature, value=%.2f", m_mock_InsideAirTemperature);
    return m_mock_InsideAirTemperature;
  }

  int CurrentHour() const
  {
    Log().Trace("Mock: CurrentHour, value=%d", m_mock_CurrentHour);
    return m_mock_CurrentHour;
  }

  unsigned long UptimeSeconds() const
  {
    Log().Trace("Mock: UptimeSeconds, value=%d", static_cast<int>(m_mock_UptimeSeconds));
    return m_mock_UptimeSeconds;
  }

  bool IsDaytime() const
  {
    if (m_mockOn_IsDaytime) {
      Log().Trace("Mock: IsDaytime, value=%s", m_mock_IsDaytime ? "true" : "false");
      return m_mock_IsDaytime;
    }
    
    return System::IsDaytime();
  }

  unsigned long EpochTime() const
  {
    Log().Trace("Mock: EpochTime, value=%d", static_cast<int>(m_mock_EpochTime));
    return m_mock_EpochTime;
  }

  // stubs

  void OpenWindow(float delta)
  {
    Log().Trace("Stub: OpenWindow, delta=%.2f", delta);
    m_calls_OpenWindow++;
    m_lastArg_OpenWindow_delta = delta;

    System::OpenWindow(delta);
  }

  void CloseWindow(float delta)
  {
    Log().Trace("Stub: CloseWindow, delta=%.2f", delta);
    m_calls_CloseWindow++;
    m_lastArg_CloseWindow_delta = delta;

    System::CloseWindow(delta);
  }

  void RunWindowActuator(bool extend)
  {
    Log().Trace("Stub: RunWindowActuator, extend=%s", extend ? "true" : "false");
    m_lastArg_RunWindowActuator_extend = extend;
    m_calls_RunWindowActuator++;
  }

  void StopActuator()
  {
    Log().Trace("Stub: StopActuator");
    m_calls_StopActuator++;
  }

  void Delay(unsigned long ms)
  {
    Log().Trace("Stub: Delay, extend=%d", static_cast<int>(ms));
    m_lastArg_SystemDelay_ms = ms;
    m_calls_SystemDelay++;
  }

  void ReportWarning(const char *m, ...)
  {
    Log().Trace("Stub: ReportWarning");
    m_calls_ReportWarning++;
  }

  void HandleNightToDayTransition()
  {
    Log().Trace("Stub: HandleNightToDayTransition");
    m_calls_HandleNightToDayTransition++;
  }

  void HandleDayToNightTransition()
  {
    Log().Trace("Stub: HandleDayToNightTransition");
    m_calls_HandleDayToNightTransition++;
  }
  
  // expose protected members to public

  float CalculateMoisture(float value) const { return System::CalculateMoisture(value); }
  bool ApplyWindowProgress(float value) { return System::ApplyWindowProgress(value); }

  // mock enable

  bool m_mockOn_IsDaytime;

  // mock values

  bool m_mock_ReadDhtSensor;
  float m_mock_WaterTemperature;
  float m_mock_SoilTemperature;
  float m_mock_InsideAirTemperature;
  int m_mock_CurrentHour;
  unsigned long m_mock_UptimeSeconds;
  bool m_mock_IsDaytime;
  unsigned long m_mock_EpochTime;

  // call counters (init to 0)

  int m_calls_OpenWindow;
  int m_calls_CloseWindow;
  int m_calls_RunWindowActuator;
  int m_calls_StopActuator;
  int m_calls_SystemDelay;
  int m_calls_ReportWarning;
  int m_calls_HandleNightToDayTransition;
  int m_calls_HandleDayToNightTransition;

  // last arg (leave undefined)

  float m_lastArg_OpenWindow_delta;
  float m_lastArg_CloseWindow_delta;
  bool m_lastArg_RunWindowActuator_extend;
  unsigned long m_lastArg_SystemDelay_ms;

  native::greenhouse::Heating m_heating;
  native::greenhouse::Heating &Heating() { return m_heating; }
};
