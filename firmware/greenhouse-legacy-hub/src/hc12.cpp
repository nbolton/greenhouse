#include "hc12.h"

#include "rf69.h"

#include <SoftwareSerial.h>
#include <gh_protocol.h>

#define PIN_TX PC14
#define PIN_RX PC13
#define TX_BIT_RATE 2000
#define TX_WAIT_DELAY 20
#define ERROR_DELAY 500
#define BAUD 9600

static SoftwareSerial s_hc12(PIN_TX, PIN_RX);

namespace hc12 {

static void rx();

void init() { s_hc12.begin(BAUD); }

void update() { rx(); }

void tx(char *data, int len) { s_hc12.write(data, len); }

void rx()
{
  char buf[GH_LENGTH];
  if (s_hc12.available()) {
    int len = s_hc12.readBytes(buf, GH_LENGTH);
    rf69::relay("hc12", buf, len);
  }
}

} // namespace hc12
