#include "Greenhouse.h"
#include <unity.h>

class GreenhouseTest : public Greenhouse {
public:
  virtual bool ReadDhtSensor() const { return m_mock_ReadDhtSensor; }
  virtual float Temperature() const { return m_mock_Temperature; }
  virtual float Humidity() const { return m_mock_Humidity; }
  virtual void OpenWindow() { m_calls_OpenWindow++; }
  virtual void CloseWindow() { m_calls_CloseWindow++; }

  bool m_mock_ReadDhtSensor = false;
  float m_mock_Temperature = -1;
  float m_mock_Humidity = -1;
  int m_calls_OpenWindow = 0;
  int m_calls_CloseWindow = 0;
};

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void Test_Refresh_DhtNotReady_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void process()
{
  UNITY_BEGIN();
  RUN_TEST(Test_Refresh_DhtNotReady_NothingHappens);
  UNITY_END();
}

#ifdef ARDUINO

#include <Arduino.h>
void setup()
{
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  process();
}

void loop()
{
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(500);
}

#else

int main(int argc, char **argv)
{
  process();
  return 0;
}

#endif
