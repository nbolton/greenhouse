#include "io.h"

#include <trace.h>

#include "common.h"

#define IO_HIGH_EN
#define IO_LOW_EN
#define IO_DELAY 1000
#define BEEP_LENGTH 50
#define BEEP_PAUSE 200

// Address pins all LOW = 20/38
// PCF8574 pcf8574(0x20); // N
// PCF8574 pcf8574(0x21); // N
PCF8574 pcf8574_0(0x38); // A
PCF8574 pcf8574_1(0x39); // A

bool ports[IO_DEVICES][IO_PORTS];
bool beginOk = false;

namespace io {

bool init()
{
  bool ok = true;

  for (int d = 0; d < IO_DEVICES; d++) {
    for (int p = 0; p < IO_PORTS; p++) {
      ports[d][p] = true;
    }
  }

  return ok;
}

bool begin(PCF8574 &pcf8574, int i)
{
  TRACE_VA(TRACE_INFO, "init pcf8574 #%d", i);

  for (int p = 0; p < IO_PORTS; p++) {
    bool v = ports[i][p];
    TRACE_VA(TRACE_DEBUG2, "mode d=%d p=%d v=%d", i, p, (int)v);
    pcf8574.pinMode(p, OUTPUT, v);
  }

  if (!pcf8574.begin()) {
    TRACE_VA(TRACE_ERROR, "failed init pcf8574 #%d", i);
    return false;
  }

  return true;
}

bool begin()
{
  bool ok = true;

  if (!begin(pcf8574_0, 0)) {
    ok = false;
  }

  if (!begin(pcf8574_1, 1)) {
    ok = false;
  }

  beginOk = ok;
  return ok;
}

void set(int d, int p, bool value)
{
  ports[d][p] = value;
  if (beginOk) {
    device(d).digitalWrite(p, value);
  }
}

PCF8574 &device(int i)
{
  switch (i) {
  case 0: {
    return pcf8574_0;
  }
  case 1: {
    return pcf8574_1;
  }
  default: {
    TRACE(TRACE_ERROR, "invalid i/o");
    while (true) {
      delay(1);
    }
  }
  }
}

void toggle(int d, int p)
{
  if (d < 0 || d > IO_DEVICES - 1) {
    TRACE(TRACE_ERROR, "invalid device");
    return;
  }
  if (p < 0 || p > IO_PORTS - 1) {
    TRACE(TRACE_ERROR, "invalid port");
    return;
  }
  PCF8574 &pcf8574 = device(d);
  bool old = ports[d][p];
  ports[d][p] = !old;
  TRACE_VA(TRACE_DEBUG1, "toggle from %d to %d", (int)old, (int)ports[d][p]);
  pcf8574.digitalWrite(p, ports[d][p]);
}

void blink()
{
  blink(pcf8574_0, 0);
  blink(pcf8574_1, 1);
}

void blink(PCF8574 &pcf8574, int i)
{
#ifdef IO_LOW_EN

  TRACE_VA(TRACE_INFO, "blink device #%d: low", i);

  digitalWrite(LED_BUILTIN, LOW);
  pcf8574.digitalWrite(P0, LOW);
  pcf8574.digitalWrite(P1, LOW);
  pcf8574.digitalWrite(P2, LOW);
  pcf8574.digitalWrite(P3, LOW);
  pcf8574.digitalWrite(P4, LOW);
  pcf8574.digitalWrite(P5, LOW);
  pcf8574.digitalWrite(P6, LOW);
  pcf8574.digitalWrite(P7, LOW);
  delay(IO_DELAY);

#endif

#ifdef IO_HIGH_EN

  TRACE_VA(TRACE_INFO, "blink device #%d: high", i);

  digitalWrite(LED_BUILTIN, HIGH);
  pcf8574.digitalWrite(P0, HIGH);
  pcf8574.digitalWrite(P1, HIGH);
  pcf8574.digitalWrite(P2, HIGH);
  pcf8574.digitalWrite(P3, HIGH);
  pcf8574.digitalWrite(P4, HIGH);
  pcf8574.digitalWrite(P5, HIGH);
  pcf8574.digitalWrite(P6, HIGH);
  pcf8574.digitalWrite(P7, HIGH);
  delay(IO_DELAY);

#endif
}

void beep(int times)
{
  PCF8574 &pcf8574 = device(IO_DEV);
  for (int i = 0; i < times; i++) {
    pcf8574.digitalWrite(BEEP_EN, LOW);
    delay(BEEP_LENGTH);
    pcf8574.digitalWrite(BEEP_EN, HIGH);
    if (i != times - 1) {
      delay(BEEP_PAUSE);
    }
  }
}

} // namespace io
