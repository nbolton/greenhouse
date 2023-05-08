#include "command.h"

#include <Arduino.h>
#include <trace.h>

#include "io.h"
#include "power.h"

#define SERIAL_BUF_LEN 4
#define ASCII_BS 0x08
#define ASCII_LF 0x0A
#define ASCII_CR 0x0D
#define ASCII_ESC 0x1B
#define ASCII_TAB 0x09

namespace command {

char serialBuf[SERIAL_BUF_LEN] = {0};
int serialBufIndex = 0;
char lastBuf[SERIAL_BUF_LEN] = {0};
int lastBufIndex = 0;

void handleCommand();
void printHelp();

void prompt() { Serial.print("> "); }

void processSerial()
{
  while (Serial.available()) {
    const char r = Serial.read();
    TRACE_VA(TRACE_DEBUG2, "serial read: r=0x%02X", r);

    if (r == ASCII_BS) {
      serialBufIndex--;
      if (serialBufIndex < 0) {
        serialBufIndex = 0;
      }
      else {
        Serial.print(r);
      }
    }
    else if ((r == ASCII_LF) || (r == ASCII_CR)) {
      if (serialBufIndex > 0) {
        TRACE_VA(TRACE_DEBUG2, "serialBufIndex=%d", serialBufIndex);
        Serial.println();
        handleCommand();
        Serial.print(F("> "));
        memcpy(lastBuf, serialBuf, sizeof serialBuf);
        memset(serialBuf, 0, sizeof serialBuf);
        lastBufIndex = serialBufIndex;
        serialBufIndex = 0;
      }
    }
    else if (r == ASCII_TAB) {
      TRACE_VA(TRACE_DEBUG2, "serial last: %s", lastBuf);
      serialBufIndex = lastBufIndex;
      memcpy(serialBuf, lastBuf, sizeof lastBuf);
      Serial.print(serialBuf);
    }
    else if (serialBufIndex < SERIAL_BUF_LEN) {
      Serial.print(r);
      serialBuf[serialBufIndex++] = r;
    }
  }
}

void unknown()
{
  TRACE_VA(TRACE_PRINT, "unrecognized: %s", serialBuf);
  TRACE_VA(TRACE_PRINT, "help: h", serialBuf);
}

void printHelp()
{
  Serial.print(F( //
    "\nspecial:\n"
    "TAB          recall last command\n"

    "\ncommands:\n"
    "h            show help\n"
    "l[n]         log level n\n"
    "x            restart\n"
    "\n"
    "i/o devices:\n"
    "ib           blink all i/o ports\n"
    "it[dp]       toggle i/o `d` port `p`\n"
    "\n"
    "power:\n"
    "ps           power switch (batt/psu)\n"
    "pv           measure voltages\n"
    "pm           measure currents\n"
    "\n"
    "type command followed by <enter>\n"));
}

void handleIo()
{
  switch (serialBuf[1]) {
  default: {
    unknown();
    break;
  }

  case 'b': {
    io::blink();
    break;
  }

  case 't': {
    int device = serialBuf[2] - '0';
    int port = serialBuf[3] - '0';
    TRACE_VA(TRACE_DEBUG1, "io=%d port=%d", device, port);
    io::toggle(device, port);
    break;
  }
  }
}

void handleCurrent()
{
  switch (serialBuf[1]) {
  default: {
    unknown();
    break;
  }
  }
}

void handelPower()
{
  switch (serialBuf[1]) {
  default: {
    unknown();
    break;
  }

  case 's': {
    power::toggleSource();
    break;
  }

  case 'v': {
    power::printVoltage();
    break;
  }

  case 'c': {
    power::printCurrent();
    break;
  }
  }
}

void handleCommand()
{
  switch (serialBuf[0]) {
  default: {
    unknown();
    break;
  }

  case 'h':
  case '?': {
    printHelp();
    break;
  }

  case 'x': {
    TRACE(TRACE_PRINT, "restarting");
    ESP.restart();
    break;
  }

  case 'l': {
    const char levelChar = serialBuf[1];
    const int level = levelChar - '0';
    if ((level >= TRACE_ERROR) && (level <= TRACE_DEBUG2)) {
      shared::traceLevel(level);
      TRACE_VA(TRACE_PRINT, "log level: %s", shared::traceLevelCStr());
    }
    else if (levelChar != 0) {
      TRACE_VA(TRACE_ERROR, "invalid level: %c", levelChar);
    }
    else {
      TRACE(TRACE_ERROR, "log level not provided");
    }
    break;
  }

  case 'i': {
    handleIo();
    break;
  }

  case 'p': {
    handelPower();
    break;
  }
  }
}

} // namespace command
