
#include "motor.h"

#include <gh_protocol.h>

#include "attiny.h"
#include "pins.h"
#include "sr.h"

#define MOTOR_PWM 200

#if MOTOR_EN

unsigned long motorStop = 0;

void testMotor();

void motor_init() {
  pinMode(PIN_MOTOR_PWM, OUTPUT);
  motor_speed(MOTOR_PWM);
}

void motor_loop() {
#if MOTOR_TEST
  testMotor();
#else

  if (millis() >= motorStop) {
    clear(SR_PIN_MOTOR_A);
    clear(SR_PIN_MOTOR_B);
    shift();
  }

#endif  // MOTOR_TEST
}

void motor_speed(byte pwm) {
  analogWrite(PIN_MOTOR_PWM, pwm);
}

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

#if MOTOR_TEST

void testMotor() {
  const int pwmFrom = 100;  //(255.0f / 0.9f);
  const int pwmTo = 255;
  const int pwmDelta = 10;
  int d = 2000;
  int pwm = pwmFrom;
  bool forward = true;

  while (true) {
    analogWrite(PIN_MOTOR_PWM, pwm);

    clear(SR_PIN_MOTOR_B);
    set(SR_PIN_MOTOR_A);
    shift();
    delay(d);

    clear(SR_PIN_MOTOR_A);
    shift();
    delay(d);

    clear(SR_PIN_MOTOR_A);
    set(SR_PIN_MOTOR_B);
    shift();
    delay(d);

    clear(SR_PIN_MOTOR_B);
    shift();
    delay(d);

    if (forward) {
      pwm += pwmDelta;
    } else {
      pwm -= pwmDelta;
    }

    if (pwm >= pwmTo) {
      pwm = pwmTo;
      forward = false;
    }

    if (pwm <= pwmFrom) {
      break;
    }
  }
}

#endif  // MOTOR_TEST
#endif  // MOTOR_EN
