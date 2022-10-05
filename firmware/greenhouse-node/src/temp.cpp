#if TEMP_EN

#include "temp.h"

#include <OneWire.h>

#include "pins.h"

#define OW_MAX_DEVS 4
#define OW_ADDR_LEN 8
#define OW_DELAY 750  // or 1000?
#define OW_DS18B20_CONVERT 0x44
#define OW_READ_SCRATCH 0xBE
#define OW_ALL_DEVS 0xCC  // aka skip/ignore
#define OW_DATA_LEN 2
#define TEMP_UNKNOWN 255
#define READ_FREQ 10000  // 10s

static OneWire ow(PIN_ONE_WIRE);
static byte addrs[OW_MAX_DEVS][OW_ADDR_LEN];
static byte data[OW_MAX_DEVS][OW_DATA_LEN];
static byte devs;
static unsigned long nextRead = 0;

void scan();
void read(int dev);

void temp_loop() {
  if (millis() >= nextRead) {
    scan();
    for (int i = 0; i < devs; i++) {
      read(i);
    }
    nextRead = millis() + READ_FREQ;
  }
}

byte temp_devs() { return devs; }

byte temp_data(byte dev, byte part) { return data[dev][part]; }

void scan() {
  for (devs = 0; devs < OW_MAX_DEVS; devs++) {
    if (!ow.search(addrs[devs])) {
      break;
    }
  }
  ow.reset_search();
}

void read(int dev) {
  byte* addr = addrs[dev];
  data[dev][0] = TEMP_UNKNOWN;
  data[dev][1] = TEMP_UNKNOWN;

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

  data[dev][0] = ow.read();
  data[dev][1] = ow.read();

  // HACK: ignore the rest of the data
  // TODO: is this really needed?
  // TODO: if everything seems fine, remove this.
  // delay(100);
}

#endif  // TEMP_EN
