#include "rf95.h"

#include "common.h"
#include "rf69.h"

#include <Arduino.h>
#include <RH_RF95.h>

#define PIN_CS PA3
#define PIN_IRQ PA4
#define SOURCE "rf95"

static RH_RF95 s_rf95(PIN_CS, PIN_IRQ);

namespace rf95 {

static void listen();

void init()
{
  if (!s_rf95.init()) {
    halt();
  }
}

void update() { listen(); }

void tx(char *data, int len)
{
  s_rf95.send((const uint8_t *)data, sizeof(data));
  s_rf95.waitPacketSent();
}

static void listen()
{
  uint8_t buf[RH_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (s_rf95.available()) {
    if (s_rf95.recv(buf, &len)) {
      char *bufChar = (char *)buf;
      relay(SOURCE, bufChar, len);
    }
  }
}

} // namespace rf95
