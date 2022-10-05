#pragma once

#include "attiny.h"

#if TEMP_EN

void temp_loop();
byte temp_devs();
byte temp_data(byte dev, byte part);

#endif // TEMP_EN
