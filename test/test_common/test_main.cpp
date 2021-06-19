#include <unity.h>
#include "Greenhouse.h"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

class Blynk : public IBlynk { };
class Sensors : public ISensors { };

void test_hello_world(void)
{
    Blynk blynk;
    Sensors sensors;
    Greenhouse greenhouse(blynk, sensors);
    greenhouse.Run();
}

void process()
{
    UNITY_BEGIN();
    RUN_TEST(test_hello_world);
    UNITY_END();
}

#ifdef ARDUINO

#include <Arduino.h>
void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    process();
}

void loop() {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(500);
}

#else

int main(int argc, char **argv) {
    process();
    return 0;
}

#endif
