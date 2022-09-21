// caution!
// dumbness in iotnx4.h (provided by platformio):
// .platformio\packages\toolchain-atmelavr\avr\include\avr\iotnx4.h
// the header defines P[AB][n] with all the wrong pin numbers, but also has an
// obscure #warning that tells you that the pins are counterclockwise (i.e.
// don't match the pins just defined by the header... wtf?)
// ATMEL ATTINY84 / ARDUINO
//
//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D  0)  PB0  2|    |13  PA0  (D 10)        AREF
//             (D  1)  PB1  3|    |12  PA1  (D  9)
//             (D 11)  PB3  4|    |11  PA2  (D  8)
//  PWM  INT0  (D  2)  PB2  5|    |10  PA3  (D  7)
//  PWM        (D  3)  PA7  6|    |9   PA4  (D  6)
//  PWM        (D  4)  PA6  7|    |8   PA5  (D  5)        PWM
//                           +----+
#include <Arduino.h>
#undef PA0
#undef PA1
#undef PA2
#undef PA3
#undef PA4
#undef PA5
#undef PA6
#undef PA7
#undef PB0
#undef PB1
#undef PB2
#undef PB3
#define PA0 10
#define PA1 9
#define PA2 8
#define PA3 7
#define PA4 6
#define PA5 5
#define PA6 4
#define PA7 3
#define PB0 0
#define PB1 1
#define PB2 2
#define PB3 11

#undef NULL
#define NULL 0
