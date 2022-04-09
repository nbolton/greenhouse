#include "test.h"

#include "../native/greenhouse/Time.h"
#include "TestSystem.h"
#include "TestTime.h"

#include <unity.h>

using namespace native::greenhouse;

void Test_IsDaytime_CurrentHourBeforeDayStart_ReturnsFalse()
{
  TestTime time;
  time.m_mock_CurrentHour = 0;
  time.DayStartHour(1);
  time.DayEndHour(3);
  TEST_ASSERT_EQUAL(false, time.IsDaytime());
}

void Test_IsDaytime_CurrentHourEqualsDayStart_ReturnsTrue()
{
  TestTime time;
  time.m_mock_CurrentHour = 1;
  time.DayStartHour(1);
  time.DayEndHour(3);
  TEST_ASSERT_EQUAL(true, time.IsDaytime());
}

void Test_IsDaytime_CurrentHourInBounds_ReturnsTrue()
{
  TestTime time;
  time.m_mock_CurrentHour = 2;
  time.DayStartHour(1);
  time.DayEndHour(3);
  TEST_ASSERT_EQUAL(true, time.IsDaytime());
}

void Test_IsDaytime_CurrentHourEqualsDayEnd_ReturnsFalse()
{
  TestTime time;
  time.m_mock_CurrentHour = 3;
  time.DayStartHour(1);
  time.DayEndHour(3);
  TEST_ASSERT_EQUAL(false, time.IsDaytime());
}

void Test_IsDaytime_CurrentHourAfterDayEnd_ReturnsFalse()
{
  TestTime time;
  time.m_mock_CurrentHour = 4;
  time.DayStartHour(1);
  time.DayEndHour(3);
  TEST_ASSERT_EQUAL(false, time.IsDaytime());
}

void Test_Refresh_NighttimeAndNoTransitionYet_DayNightTransition(void)
{
  TestTime time;
  TestSystem system;
  time.System(system);

  time.DayStartHour(2);
  time.DayEndHour(3);

  time.m_mock_CurrentHour = 1;
  time.m_mock_EpochTime = 3600; // 1h

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  // no transition happened yet, so day to night should happen
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);
}

void Test_Refresh_NighttimeStartOfDay_NightDayTransition(void)
{
  TestTime time;
  TestSystem system;
  time.System(system);

  time.DayStartHour(2);
  time.DayEndHour(3);

  time.m_mock_CurrentHour = 2;
  time.m_mock_EpochTime = 3600 * 2; // 2h

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);
}

// Test_Refresh_NighttimeAndNoTransitionYet_DayNightTransition
// Test_Refresh_NighttimeStartOfDay_NightDayTransition

void Test_Refresh_NighttimeStartOfDay_NightDayTransition2(void)
{
  TestTime time;
  TestSystem system;
  time.System(system);

  time.DayStartHour(2);
  time.DayEndHour(3);

  time.m_mock_CurrentHour = 2;
  time.m_mock_EpochTime = 3600 * 2; // 2h

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);
}

void Test_Refresh_DayNightDayNight_DayNightTransitionedTwice(void)
{
  TestTime time;
  TestSystem system;
  time.System(system);

  time.m_mock_CurrentHour = 2;
  time.m_mock_EpochTime = 3600 * 2; // 2h

  time.DayStartHour(2);
  time.DayEndHour(3);

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_HandleDayToNightTransition);

  time.m_mock_CurrentHour = 1;
  time.m_mock_EpochTime = 3600 * 25; // 1d 1h

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);

  time.m_mock_CurrentHour = 2;
  time.m_mock_EpochTime = 3600 * 26; // 1d 2h

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);

  time.m_mock_CurrentHour = 1;
  time.m_mock_EpochTime = 3600 * 49; // 2d 1h

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleDayToNightTransition);
}

void Test_Refresh_LastNightTransitionSameDay_TransitionedToNight(void)
{
  TestTime time;
  TestSystem system;
  time.System(system);

  time.m_mock_CurrentHour = 17;
  time.m_mock_EpochTime = 17 * 3600;
  time.DayEndHour(17);

  time.DayToNightTransitionTime(0);

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(17 * 3600, time.DayToNightTransitionTime());
}

void Test_Refresh_LastNightTransitionSameDay_NextHourNoTransition(void)
{
  TestTime time;
  TestSystem system;
  time.System(system);

  time.m_mock_CurrentHour = 23;
  time.m_mock_EpochTime = 1649365645 + 3600;

  time.NightToDayTransitionTime(1649314854);
  time.DayToNightTransitionTime(1649365645);

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_HandleDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(1649365645, time.DayToNightTransitionTime());
}

void Test_Refresh_LastNightTransitionSameDay_NextDayNoTransition(void)
{
  TestTime time;
  TestSystem system;
  time.System(system);

  time.m_mock_CurrentHour = 00;
  time.m_mock_EpochTime = 1649365645 + (3600 * 2);

  time.NightToDayTransitionTime(1649314854);
  time.DayToNightTransitionTime(1649365645);

  time.CheckTimeTransition();
  time.CheckTimeTransition();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_HandleDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(1649365645, time.DayToNightTransitionTime());
}

void testTime()
{
  //RUN_TEST(Test_Refresh_NightDayNightDay_DayNightTransitionedTwice);
  RUN_TEST(Test_Refresh_DayNightDayNight_DayNightTransitionedTwice);
    return;
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_TransitionedToNight);
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_NextHourNoTransition);
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_NextDayNoTransition);
  return;

  RUN_TEST(Test_IsDaytime_CurrentHourBeforeDayStart_ReturnsFalse);
  RUN_TEST(Test_IsDaytime_CurrentHourEqualsDayStart_ReturnsTrue);
  RUN_TEST(Test_IsDaytime_CurrentHourInBounds_ReturnsTrue);
  RUN_TEST(Test_IsDaytime_CurrentHourEqualsDayEnd_ReturnsFalse);
  RUN_TEST(Test_IsDaytime_CurrentHourAfterDayEnd_ReturnsFalse);
}
