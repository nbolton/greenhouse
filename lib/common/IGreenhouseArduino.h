#pragma once

class IGreenhouseArduino {
  public:
    virtual void Setup() = 0;
    virtual void Loop() = 0;
};
