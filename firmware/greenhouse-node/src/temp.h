#pragma once

#include "attiny.h"

#if TEMP_EN

#if OW_SINGLE
void readTemp();
#else
byte scanTempDevs();
void readTempDev(int addrIndex);
#endif // OW_SINGLE

byte tempData(byte i);

#endif // TEMP_EN
