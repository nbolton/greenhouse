#pragma once

#include "Log.h"

enum windowState {
  windowUnknown, windowOpen, windowClosed
};

class Greenhouse
{
    public:
        Greenhouse();
        virtual void Setup();
        virtual void Loop();
        virtual bool Refresh();
    
    protected:
        virtual const Log& Log() const { return m_log; }
        virtual float GetTemperature() const { return -1; }
        virtual float GetHumidity() const { return -1; }
        virtual bool IsDhtReady() const { return false; }
        virtual void ReportTemperature(float t) { }
        virtual void ReportHumidity(float h) { }
        virtual void FlashLed(int times) { }
        virtual void OpenWindow() { }
        virtual void CloseWindow() { }

    private:
        ::Log m_log;
        bool m_dhtFailSent;
        bool m_autoMode;
        int m_openTemp;
        windowState m_ws;
};
