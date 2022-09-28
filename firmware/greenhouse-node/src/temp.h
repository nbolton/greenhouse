#pragma once

#include "attiny.h"

#if TEMP_EN

byte scanTempDevs();
void readTempDev(int addrIndex);
byte tempData(byte i);

#endif // TEMP_EN
