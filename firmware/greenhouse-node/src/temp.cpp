#if TEMP_EN

#include <OneWire.h>

#include "pins.h"

#define OW_MAX_DEVS 4
#define OW_ADDR_LEN 8
#define OW_DELAY 750  // or 1000?
#define OW_DS18B20_CONVERT 0x44
#define OW_READ_SCRATCH 0xBE
#define OW_ALL_DEVS 0xCC  // aka skip/ignore
#define TEMP_UNKNOWN 255

static OneWire ow(PIN_ONE_WIRE);
static byte data[2];

#if !OW_SINGLE
byte addrs[OW_MAX_DEVS][OW_ADDR_LEN];
#endif

byte tempData(byte i) { return data[i]; }

#if OW_SINGLE

// only supports 1 device on the wire
void readTemp() {
  ow.reset();
  ow.write(OW_ALL_DEVS);
  ow.write(OW_DS18B20_CONVERT);
  delay(OW_DELAY);
  ow.reset();
  ow.write(OW_ALL_DEVS);
  ow.write(OW_READ_SCRATCH);
  data[0] = ow.read();
  data[1] = ow.read();
}

#else  // many devs

byte scanTempDevs() {
  byte devs;
  for (devs = 0; devs < OW_MAX_DEVS; devs++) {
    if (!ow.search(addrs[devs])) {
      break;
    }
  }
  ow.reset_search();
  return devs;
}

void readTempDev(int addrIndex) {
  byte* addr = addrs[addrIndex];
  data[0] = TEMP_UNKNOWN;
  data[1] = TEMP_UNKNOWN;

  // tell DS18B20 to take a temperature reading and put it on the scratchpad.
  if (!ow.reset()) {
    return;
  }
  ow.select(addr);
  ow.write(OW_DS18B20_CONVERT);

  // wait for DS18B20 to do it's thing.
  delay(OW_DELAY);

  // tell DS18B20 to read from it's scratchpad.
  if (!ow.reset()) {
    return;
  }
  ow.select(addr);
  ow.write(OW_READ_SCRATCH);

  data[0] = ow.read();
  data[1] = ow.read();

  // HACK: ignore the rest of the data
  delay(100);
}

#endif  // OW_SINGLE
#endif  // TEMP_EN
