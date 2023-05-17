#include "test.h"

#include "TestSystem.h"
#include "TestTime.h"

#include <unity.h>

using namespace greenhouse::native;

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

void Test_CheckTransition_NighttimeAndNoTransitionYet_DayToNightTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 0;
  time.m_mock_EpochTime = 0; // 12am

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(1, time.m_calls_OnDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnNightToDayTransition);
}

void Test_CheckTransition_DaytimeAndNoTransitionYet_NightToDayTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 10;
  time.m_mock_EpochTime = 3600 * 10; // 10am

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(1, time.m_calls_OnNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnDayToNightTransition);
}

void Test_CheckTransition_DaytimeStartOfDay_NightToDayTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 9;
  time.m_mock_EpochTime = 3600 * 9; // 9am
  time.NightToDayTransitionTime(0); // 12am

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(1, time.m_calls_OnNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnDayToNightTransition);
}

void Test_CheckTransition_DaytimeBetweenHours_NoTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 10;
  time.m_mock_EpochTime = 3600 * 10; // 10am
  time.NightToDayTransitionTime(0); // 12am

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnDayToNightTransition);
}

void Test_CheckTransition_DaytimeEndOfDay_DayToNightTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 17;
  time.m_mock_EpochTime = 3600 * 17; // 5pm
  time.DayToNightTransitionTime(0); // 12am

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(1, time.m_calls_OnDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnNightToDayTransition);
}

void Test_CheckTransition_NightTimeNextHour_NoTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 18;
  time.m_mock_EpochTime = 3600 * 18; // 6pm
  time.DayToNightTransitionTime(3600 * 17); // 5pm

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnNightToDayTransition);
}

void Test_CheckTransition_NightTimeNextDay_NoTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 0;
  time.m_mock_EpochTime = 3600 * 24; // 12am
  time.DayToNightTransitionTime(3600 * 17); // 5pm

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnNightToDayTransition);
}

void Test_CheckTransition_DaytimeNextDayStartOfDay_NightToDayTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 9;
  time.m_mock_EpochTime = 3600 * (9 + 24); // +1d 9am
  time.NightToDayTransitionTime(3600 * 9); // +0d 9am

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(1, time.m_calls_OnNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnDayToNightTransition);
}

void Test_CheckTransition_DaytimeNextDayBetweenHours_NoTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 10;
  time.m_mock_EpochTime = 3600 * (10 + 24); // +1d 10am
  time.NightToDayTransitionTime(3600 * (9 + 24)); // +1d 9am

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnDayToNightTransition);
}

void Test_CheckTransition_DaytimeNextDayEndOfDay_DayToNightTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 17;
  time.m_mock_EpochTime = 3600 * (17 + 24); // +1d 5pm
  time.DayToNightTransitionTime(3600 * 17); // +0d 5pm

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(1, time.m_calls_OnDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnNightToDayTransition);
}

void Test_CheckTransition_DaytimeTransitionOverdue_NightToDayTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 10;
  time.m_mock_EpochTime = 3600 * (10 + 24); // +1d 10am
  time.DayToNightTransitionTime(3600 * 9); // +0d 9am

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(1, time.m_calls_OnNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnDayToNightTransition);
}

void Test_CheckTransition_NighttimeTransitionOverdue_DayToNightTransition(void)
{
  TestTime time;

  time.DayStartHour(9);
  time.DayEndHour(17);

  time.m_mock_CurrentHour = 18;
  time.m_mock_EpochTime = 3600 * (18 + 24); // +1d 5pm
  time.DayToNightTransitionTime(3600 * 17); // +0d 5pm

  time.CheckTransition();
  time.CheckTransition();

  TEST_ASSERT_EQUAL_INT(1, time.m_calls_OnDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(0, time.m_calls_OnNightToDayTransition);
}

void testTime()
{
  RUN_TEST(Test_CheckTransition_NighttimeAndNoTransitionYet_DayToNightTransition);
  RUN_TEST(Test_CheckTransition_DaytimeAndNoTransitionYet_NightToDayTransition);
  RUN_TEST(Test_CheckTransition_DaytimeStartOfDay_NightToDayTransition);
  RUN_TEST(Test_CheckTransition_DaytimeBetweenHours_NoTransition);
  RUN_TEST(Test_CheckTransition_DaytimeEndOfDay_DayToNightTransition);
  RUN_TEST(Test_CheckTransition_NightTimeNextHour_NoTransition);
  RUN_TEST(Test_CheckTransition_NightTimeNextDay_NoTransition);
  RUN_TEST(Test_CheckTransition_DaytimeNextDayStartOfDay_NightToDayTransition);
  RUN_TEST(Test_CheckTransition_DaytimeNextDayBetweenHours_NoTransition);
  RUN_TEST(Test_CheckTransition_DaytimeNextDayEndOfDay_DayToNightTransition);
  RUN_TEST(Test_CheckTransition_DaytimeTransitionOverdue_NightToDayTransition);
  RUN_TEST(Test_CheckTransition_NighttimeTransitionOverdue_DayToNightTransition);
  RUN_TEST(Test_IsDaytime_CurrentHourBeforeDayStart_ReturnsFalse);
  RUN_TEST(Test_IsDaytime_CurrentHourEqualsDayStart_ReturnsTrue);
  RUN_TEST(Test_IsDaytime_CurrentHourInBounds_ReturnsTrue);
  RUN_TEST(Test_IsDaytime_CurrentHourEqualsDayEnd_ReturnsFalse);
  RUN_TEST(Test_IsDaytime_CurrentHourAfterDayEnd_ReturnsFalse);
}
