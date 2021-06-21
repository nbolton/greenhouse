#include "GreenhouseArduino.h"

#include <Arduino.h>
#include <unity.h>

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void test_hello_world(void)
{
  GreenhouseArduino greenhouse;
  GreenhouseArduino::Instance(greenhouse);

  greenhouse.Setup();
  greenhouse.Loop();
}

void process()
{
  UNITY_BEGIN();
  RUN_TEST(test_hello_world);
  UNITY_END();
}

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
