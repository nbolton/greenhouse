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
    m_calls_SetWindowActuatorSpeed(0),
    m_calls_SystemDelay(0),
    m_calls_ReportWarning(0),

    // mocks
    m_mock_ReadDhtSensor(false),
    m_mock_WaterTemperature(0),
    m_mock_SoilTemperature(0),
    m_mock_InsideAirTemperature(0),
    m_mock_CurrentHour(0),
    m_mock_UptimeSeconds(0)
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
    Log().Trace("Mock: UptimeSeconds, value=%d", m_mock_UptimeSeconds);
    return m_mock_UptimeSeconds;
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

  void RunWindowActuator(bool forward)
  {
    Log().Trace("Stub: RunWindowActuator, forward=%s", forward ? "true" : "false");
    m_lastArg_RunWindowActuator_forward = forward;
    m_calls_RunWindowActuator++;
  }

  void StopActuator()
  {
    Log().Trace("Stub: StopActuator");
    m_calls_StopActuator++;
  }

  void SetWindowActuatorSpeed(int speed)
  {
    Log().Trace("Stub: SetWindowActuatorSpeed, forward=%d", speed);
    m_lastArg_SetWindowActuatorSpeed_speed = speed;
    m_calls_SetWindowActuatorSpeed++;
  }

  void SystemDelay(unsigned long ms)
  {
    Log().Trace("Stub: SystemDelay, forward=%d", ms);
    m_lastArg_SystemDelay_ms = ms;
    m_calls_SystemDelay++;
  }

  void ReportWarning(const char *m, ...)
  {
    Log().Trace("Stub: ReportWarning");
    m_calls_ReportWarning++;
  }
  
  // expose protected members to public

  float CalculateMoisture(float value) const { return System::CalculateMoisture(value); }
  bool ApplyWindowProgress(float value) { return System::ApplyWindowProgress(value); }

  // mock values (leave undefined)

  bool m_mock_ReadDhtSensor;
  float m_mock_WaterTemperature;
  float m_mock_SoilTemperature;
  float m_mock_InsideAirTemperature;
  int m_mock_CurrentHour;
  unsigned long m_mock_UptimeSeconds;

  // call counters (init to 0)

  int m_calls_OpenWindow;
  int m_calls_CloseWindow;
  int m_calls_RunWindowActuator;
  int m_calls_StopActuator;
  int m_calls_SetWindowActuatorSpeed;
  int m_calls_SystemDelay;
  int m_calls_ReportWarning; 

  // last arg (leave undefined)

  float m_lastArg_OpenWindow_delta;
  float m_lastArg_CloseWindow_delta;
  bool m_lastArg_RunWindowActuator_forward;
  float m_lastArg_SetWindowActuatorSpeed_speed;
  unsigned long m_lastArg_SystemDelay_ms;

  native::greenhouse::Heating m_heating;
  native::greenhouse::Heating &Heating() { return m_heating; }
};
