#include "trace.h"

#include <WString.h>

#if TRACE_EN

#define SERIAL_DELAY 100
#define PRINT_BUF_LEN 200
#define LEVEL_BUF_LEN 10

#ifndef TRACE_LEVEL
#define TRACE_LEVEL TRACE_INFO
#endif

namespace shared {

char s_buffer[PRINT_BUF_LEN] = {0};
int s_preLen = 0;
int s_level = -1;
char s_levelCStr[LEVEL_BUF_LEN] = {0};
bool s_init = false;

void traceInit()
{
  Serial.begin(9600);
  Serial.setTimeout(2000);
  while (!Serial) {
    delay(1);
  }

  // wait a while for serial monitor to connect
  delay(SERIAL_DELAY);

  Serial.println("");
  traceLevel(TRACE_LEVEL);
  s_init = true;
}

void traceLevel(int level) { s_level = level; }

const char *traceLevelCStr() { return traceLevelCStr(s_level); }

const char *traceLevelCStr(int level)
{
  switch (level) {
  case TRACE_ERROR: {
    sprintf(s_levelCStr, "ERROR");
    break;
  }
  case TRACE_WARN: {
    sprintf(s_levelCStr, "WARN");
    break;
  }
  case TRACE_INFO: {
    sprintf(s_levelCStr, "INFO");
    break;
  }
  case TRACE_DEBUG1: {
    sprintf(s_levelCStr, "DEBUG1");
    break;
  }
  case TRACE_DEBUG2: {
    sprintf(s_levelCStr, "DEBUG2");
    break;
  }
  default: {
    sprintf(s_levelCStr, "?");
    break;
  }
  }
  return s_levelCStr;
}

void trace(
  int level, const __FlashStringHelper *file, int line, const __FlashStringHelper *format, ...)
{
  if (!s_init) {
    traceInit();
  }

  const char *fileCS = (const char *)file;
  const char *fileName = (strrchr(fileCS, '/') ? strrchr(fileCS, '/') + 1 : fileCS);

  traceLevelCStr(level);

  if (level > s_level) {
    return;
  }

  char *buf;
  int len;
  if (level > TRACE_PRINT) {
    sprintf(s_buffer, "[%lu][%-6s][%s:%d] ", millis(), s_levelCStr, fileName, line);
    s_preLen = strlen(s_buffer);

    buf = s_buffer + s_preLen;
    len = sizeof(s_buffer) - s_preLen;
  }
  else {
    buf = s_buffer;
    len = sizeof(s_buffer);
  }

  va_list args;
  va_start(args, (PGM_P)format);
  vsnprintf_P(buf, len, (const char *)format, args);
  va_end(args);

  Serial.println(s_buffer);
}

} // namespace shared

#endif // TRACE_ON
