#pragma once

#define TRACE(a)
#define TRACE_F(a, ...)
#define TRACE_C(a)

#define halt()                                                                                     \
  while (1) {                                                                                      \
    delay(1);                                                                                      \
  }

const int k_unknown = -1;
const unsigned long k_unknownUL = 0;
