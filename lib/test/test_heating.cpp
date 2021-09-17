#include "test_heating.h"

#include "../native/greenhouse/Heating.h"

#include "TestSystem.h"
#include <unity.h>

using namespace native::greenhouse;

class TestHeating : public Heating {
};

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void Test_Update_DaytimeBelowDayTemp_SwitchOnWaterHeater(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DayWaterTemperature(2);
  heating.DaySoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeaterIsOn());
}

void Test_Update_DaytimeAboveDayTemp_SwitchOffWaterHeater(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_WaterTemperature = 3;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DayWaterTemperature(1);
  heating.WaterHeaterIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeaterIsOn());
}

void Test_Update_NighttimeBelowNightTemp_SwitchOnWaterHeater(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightWaterTemperature(2);
  heating.NightSoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeaterIsOn());
}

void Test_Update_NighttimeAboveNightTemp_SwitchOffWaterHeater(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_WaterTemperature = 3;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightWaterTemperature(1);
  heating.WaterHeaterIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeaterIsOn());
}

void Test_Update_DaytimeBelowDayTemp_SwitchOnSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DaySoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.SoilHeatingIsOn());
}

void Test_Update_DaytimeAboveDayTemp_SwitchOffSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 3;      // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DaySoilTemperature(1);
  heating.SoilHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
}

void Test_Update_NighttimeBelowNightTemp_SwitchOnSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightSoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.SoilHeatingIsOn());
}

void Test_Update_NighttimeAboveNightTemp_SwitchOffSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_SoilTemperature = 3;      // remember margin
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightSoilTemperature(1);
  heating.SoilHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
}

void Test_Update_DaytimeBelowDayTemp_SwitchOnAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_InsideAirTemperature = 0; // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.DayAirTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.AirHeatingIsOn());
}

void Test_Update_DaytimeAboveDayTemp_SwitchOffAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_InsideAirTemperature = 3; // remember margin
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.DayAirTemperature(1);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_NighttimeBelowNightTemp_SwitchOnAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_InsideAirTemperature = 0; // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.NightAirTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.AirHeatingIsOn());
}

void Test_Update_NighttimeAboveNightTemp_SwitchOffAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_InsideAirTemperature = 3; // remember margin
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.NightAirTemperature(1);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_SoilBelowTempAndWaterBelowTempThenWaterAboveTemp_SwitchOffWaterHeater(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 0;
  system.m_mock_WaterTemperature = 5; // remember margin + min water delta
  system.m_mock_InsideAirTemperature = 2;

  heating.DayWaterTemperature(7);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.SoilHeatingIsOn());
  TEST_ASSERT_EQUAL(true, heating.WaterHeaterIsOn());

  system.m_mock_SoilTemperature = 2; // on target, but not above/below margin
  system.m_mock_WaterTemperature = 6;
  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeaterIsOn());
}

void Test_Update_WaterOnWhenAirAndSoilHeatingOff_SwitchOffWaterHeater(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 3;
  system.m_mock_WaterTemperature = 0;
  system.m_mock_InsideAirTemperature = 3;

  heating.DayWaterTemperature(2);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);
  heating.WaterHeaterIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeaterIsOn());
}

void Test_Update_WaterHeaterRuntimeLimitReached_SwitchOffWaterHeater(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 0;
  system.m_mock_WaterTemperature = 0;
  system.m_mock_InsideAirTemperature = 0;
  system.m_mock_UptimeSeconds = 0;

  heating.DayWaterTemperature(8);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);
  heating.WaterHeaterLimitMinutes(1);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeaterIsOn());

  system.m_mock_UptimeSeconds = 60;

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeaterIsOn());

  system.m_mock_UptimeSeconds = 120;

  heating.Update();

  // ensure water heating switch on is blocked
  TEST_ASSERT_EQUAL(false, heating.WaterHeaterIsOn());
}

void Test_Update_DaytimeBelowDayTempAndWaterNotWarmEnough_SwitchOffSoilAndAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DayWaterTemperature(2);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);
  heating.WaterHeaterIsOn(false);
  heating.SoilHeatingIsOn(true);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeaterIsOn());
  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_NighttimeBelowDayTempAndWaterNotWarmEnough_SwitchOffSoilAndAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightWaterTemperature(2);
  heating.NightSoilTemperature(2);
  heating.NightAirTemperature(2);
  heating.WaterHeaterIsOn(false);
  heating.SoilHeatingIsOn(true);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeaterIsOn());
  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_DaytimeAirBelowTemp_WaterTooLowAndWaterLimitReached_SwitchOffAirHeating()
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_WaterTemperature = 21.94;     // remember margin
  system.m_mock_SoilTemperature = 21.69;      // remember margin
  system.m_mock_InsideAirTemperature = 20.04; // remember margin

  heating.DayWaterTemperature(40);
  heating.DaySoilTemperature(24);
  heating.DayAirTemperature(21);
  heating.WaterHeaterIsOn(false);
  heating.SoilHeatingIsOn(false);
  heating.AirHeatingIsOn(true);
  heating.WaterHeaterLimitMinutes(1);
  heating.WaterHeaterRuntimeMinutes(1);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeaterIsOn());
  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_HandleDayNightTransition_Called_WaterHeaterRuntimeLimitIsReset(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  heating.WaterHeaterLimitMinutes(1);
  heating.HandleDayNightTransition();

  TEST_ASSERT_EQUAL_INT(0, heating.WaterHeaterRuntimeMinutes());
}

void testHeating()
{
  RUN_TEST(Test_Update_DaytimeBelowDayTemp_SwitchOnWaterHeater);
  RUN_TEST(Test_Update_DaytimeAboveDayTemp_SwitchOffWaterHeater);
  RUN_TEST(Test_Update_NighttimeBelowNightTemp_SwitchOnWaterHeater);
  RUN_TEST(Test_Update_NighttimeAboveNightTemp_SwitchOffWaterHeater);
  RUN_TEST(Test_Update_DaytimeBelowDayTemp_SwitchOnSoilHeating);
  RUN_TEST(Test_Update_DaytimeAboveDayTemp_SwitchOffSoilHeating);
  RUN_TEST(Test_Update_NighttimeBelowNightTemp_SwitchOnSoilHeating);
  RUN_TEST(Test_Update_NighttimeAboveNightTemp_SwitchOffSoilHeating);
  RUN_TEST(Test_Update_DaytimeBelowDayTemp_SwitchOnAirHeating);
  RUN_TEST(Test_Update_DaytimeAboveDayTemp_SwitchOffAirHeating);
  RUN_TEST(Test_Update_NighttimeBelowNightTemp_SwitchOnAirHeating);
  RUN_TEST(Test_Update_NighttimeAboveNightTemp_SwitchOffAirHeating);
  RUN_TEST(Test_Update_SoilBelowTempAndWaterBelowTempThenWaterAboveTemp_SwitchOffWaterHeater);
  RUN_TEST(Test_Update_WaterOnWhenAirAndSoilHeatingOff_SwitchOffWaterHeater);
  RUN_TEST(Test_Update_WaterHeaterRuntimeLimitReached_SwitchOffWaterHeater);
  RUN_TEST(Test_Update_DaytimeBelowDayTempAndWaterNotWarmEnough_SwitchOffSoilAndAirHeating);
  RUN_TEST(Test_Update_NighttimeBelowDayTempAndWaterNotWarmEnough_SwitchOffSoilAndAirHeating);
  RUN_TEST(Test_Update_DaytimeAirBelowTemp_WaterTooLowAndWaterLimitReached_SwitchOffAirHeating);
  RUN_TEST(Test_HandleDayNightTransition_Called_WaterHeaterRuntimeLimitIsReset);
}
