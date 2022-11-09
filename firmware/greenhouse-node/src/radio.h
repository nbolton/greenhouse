#pragma once

#if RADIO_ASK || RADIO_HC12
#define RADIO_EN 1
#endif

void radio_init();
void radio_loop();
