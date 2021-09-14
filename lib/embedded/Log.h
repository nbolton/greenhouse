#pragma once

#include "../common/Log.h"

class __FlashStringHelper;

namespace embedded
{

class Log : public common::Log {
public:
  void Trace(const char *format, ...) const;
  void Trace(const __FlashStringHelper *format, ...) const;
};

}
