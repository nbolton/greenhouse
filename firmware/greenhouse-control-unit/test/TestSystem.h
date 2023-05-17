#pragma once

#include "native/System.h"

#include "TestHeating.h"
#include "TestTime.h"

using namespace greenhouse::native;

class TestSystem : public System {

public:
  TestSystem() :

    // call counters
    m_calls_OpenWindow(0),
    m_calls_CloseWindow(0),
    m_calls_RunWindowActuator(0),
    m_calls_SystemDelay(0),
    m_calls_ReportWarning(0),
    m_calls_HandleNightToDayTransition(0),
    m_calls_HandleDayToNightTransition(0),

    // mock returns
    m_mock_heating(nullptr),
    m_mock_time(nullptr),
    m_mock_ReadDhtSensor(false),
    m_mock_WaterTemperature(0),
    m_mock_SoilTemperature(0),
    m_mock_InsideAirTemperature(0)
  {
    Heating().System(*this);
    Time().System(*this);
  }

  // mocks

  bool ReadSensors()
  {
    TRACE_F("Mock: ReadSensors, value=%s", m_mock_ReadDhtSensor ? "true" : "false");
    return m_mock_ReadDhtSensor;
  }

  float WaterTemperature() const
  {
    TRACE_F("Mock: WaterTemperature, value=%.2f", m_mock_WaterTemperature);
    return m_mock_WaterTemperature;
  }

  float SoilTemperature() const
  {
    TRACE_F("Mock: SoilTemperature, value=%.2f", m_mock_SoilTemperature);
    return m_mock_SoilTemperature;
  }

  float InsideAirTemperature() const
  {
    TRACE_F("Mock: InsideAirTemperature, value=%.2f", m_mock_InsideAirTemperature);
    return m_mock_InsideAirTemperature;
  }

  greenhouse::native::Time &Time()
  {
    if (m_mock_time != nullptr) {
      TRACE("Mock: Time");
      return *m_mock_time;
    }

    // default
    return m_time;
  }

  greenhouse::native::Heating &Heating()
  {
    if (m_mock_heating != nullptr) {
      TRACE("Mock: Heating");
      return *m_mock_heating;
    }

    // default
    return m_heating;
  }

  // stubs

  void OpenWindow(float delta)
  {
    TRACE_F("Stub: OpenWindow, delta=%.2f", delta);
    m_calls_OpenWindow++;
    m_lastArg_OpenWindow_delta = delta;

    System::OpenWindow(delta);
  }

  void CloseWindow(float delta)
  {
    TRACE_F("Stub: CloseWindow, delta=%.2f", delta);
    m_calls_CloseWindow++;
    m_lastArg_CloseWindow_delta = delta;

    System::CloseWindow(delta);
  }

  void RunWindowActuator(bool extend, float delta)
  {
    TRACE_F("Stub: RunWindowActuator, extend=%s", extend ? "true" : "false");
    m_lastArg_RunWindowActuator_extend = extend;
    m_calls_RunWindowActuator++;
  }

  void Delay(unsigned long ms, const char *reason)
  {
    TRACE_F("Stub: Delay, ms=%d, reason=%s", static_cast<int>(ms), reason);
    m_lastArg_SystemDelay_ms = ms;
    m_calls_SystemDelay++;
  }

  void ReportWarning(const char *m, ...)
  {
    TRACE("Stub: ReportWarning");
    m_calls_ReportWarning++;
  }

  void HandleNightToDayTransition()
  {
    TRACE("Stub: HandleNightToDayTransition");
    m_calls_HandleNightToDayTransition++;
  }

  void HandleDayToNightTransition()
  {
    TRACE("Stub: HandleDayToNightTransition");
    m_calls_HandleDayToNightTransition++;
  }

  // mock values

  greenhouse::native::Heating *m_mock_heating;
  greenhouse::native::Time *m_mock_time;
  bool m_mock_ReadDhtSensor;
  float m_mock_WaterTemperature;
  float m_mock_SoilTemperature;
  float m_mock_InsideAirTemperature;

  // call counters (init to 0)

  int m_calls_OpenWindow;
  int m_calls_CloseWindow;
  int m_calls_RunWindowActuator;
  int m_calls_SystemDelay;
  int m_calls_ReportWarning;
  int m_calls_HandleNightToDayTransition;
  int m_calls_HandleDayToNightTransition;

  // last arg (leave undefined)

  float m_lastArg_OpenWindow_delta;
  float m_lastArg_CloseWindow_delta;
  bool m_lastArg_RunWindowActuator_extend;
  unsigned long m_lastArg_SystemDelay_ms;

  // defaults

  greenhouse::native::Heating m_heating;
  greenhouse::native::Time m_time;
};
