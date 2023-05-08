#include "sensors.h"

#include <Adafruit_SHT31.h>
#include <trace.h>

Adafruit_SHT31 insideAirSensor;
Adafruit_SHT31 outsideAirSensor;
bool insideOk, outsideOk;

#define SENSOR_ADDR_INSIDE 0x44
#define SENSOR_ADDR_OUTSIDE 0x45
// #define ERROR_EN

namespace sensors {

bool init()
{
  bool ok = true;

  TRACE(TRACE_DEBUG1, "init inside air sensor");
  insideOk = insideAirSensor.begin(SENSOR_ADDR_INSIDE);
  if (!insideOk) {
    TRACE(TRACE_ERROR, "failed init inside air sensor");
    ok = false;
  }

  TRACE(TRACE_DEBUG1, "init outside air sensor");
  outsideOk = outsideAirSensor.begin(SENSOR_ADDR_OUTSIDE);
  if (!outsideOk) {
    TRACE(TRACE_ERROR, "failed init outside air sensor");
    ok = false;
  }

#ifdef ERROR_EN
  return ok;
#else
  return true;
#endif
}

void printAir(Adafruit_SHT31 &sensor)
{
  float temp = sensor.readTemperature();
  if (!isnan(temp)) {
    TRACE_VA(TRACE_INFO, "temp: %.2f°C", temp);
  }
  else {
    TRACE(TRACE_ERROR, "invalid temp");
  }

  float humidity = sensor.readHumidity();
  if (!isnan(humidity)) {
    TRACE_VA(TRACE_INFO, "humidity: %.2f%%", humidity);
  }
  else {
    TRACE(TRACE_ERROR, "invalid humidity");
  }
}

void printAir()
{
  if (insideOk) {
    TRACE(TRACE_INFO, "inside sensor");
    printAir(insideAirSensor);
  }
  else {
    TRACE(TRACE_INFO, "inside sensor offline");
  }

  if (outsideOk) {
    TRACE(TRACE_INFO, "outside sensor");
    printAir(outsideAirSensor);
  }
  else {
    TRACE(TRACE_INFO, "outside sensor offline");
  }
}

} // namespace sensors
