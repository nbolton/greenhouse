
#include "motor.h"

#include <gh_protocol.h>

#include "attiny.h"
#include "io.h"
#include "pins.h"

#define MOTOR_PWM 200

#if MOTOR_EN

unsigned long motorStop = 0;

void testMotor();

void motor_init() {
  pinMode(PIN_MOTOR_PWM, OUTPUT);
#if MOTOR_TEST
  testMotor();
#endif
  motor_speed(MOTOR_PWM);
}

void motor_loop() {
  if (!motor_on()) {
    clear(SR_PIN_MOTOR_A);
    clear(SR_PIN_MOTOR_B);
    shift();
  }
}

void motor_speed(byte pwm) { analogWrite(PIN_MOTOR_PWM, pwm); }

bool motor_run(byte dir, byte secs) {
  motorStop = millis() + (secs * 1000);
  switch (dir) {
    case GH_MOTOR_FORWARD: {
      set(SR_PIN_MOTOR_B);
      clear(SR_PIN_MOTOR_A);
    } break;

    case GH_MOTOR_REVERSE: {
      set(SR_PIN_MOTOR_A);
      clear(SR_PIN_MOTOR_B);
    } break;

    default:
      return false;
  }
  shift();
  return true;
}

bool motor_on() { return millis() < motorStop; }

#if MOTOR_TEST

#define TEST_DELAY 200

void testMotor(int pwm) {
  analogWrite(PIN_MOTOR_PWM, pwm);

  set(SR_PIN_MOTOR_A);
  shift();
  delay(TEST_DELAY);

  clear(SR_PIN_MOTOR_A);
  shift();
  delay(TEST_DELAY);

  set(SR_PIN_MOTOR_B);
  shift();
  delay(TEST_DELAY);

  clear(SR_PIN_MOTOR_B);
  shift();
  delay(TEST_DELAY);
}

void testMotor() {
  int pwm = 64;
  for (int i = 0; i < 4; i++) {
    testMotor(pwm);
    pwm += 64;
    if (pwm > 255) {
      pwm = 255;
    }
  }
}

#endif  // MOTOR_TEST

#endif  // MOTOR_EN
