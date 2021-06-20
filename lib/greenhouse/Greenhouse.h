#pragma once

#include "Log.h"

enum WindowState
{
  windowUnknown, windowOpen, windowClosed
};

class Greenhouse
{
    public:
        Greenhouse();
        virtual void Setup();
        virtual void Loop();
        virtual bool Refresh();
        virtual const ::Log& Log() const { return m_log; }
    
    protected:
        virtual bool ReadDhtSensor() { return false; }
        virtual float Temperature() const { return -1; }
        virtual float Humidity() const { return -1; }
        virtual void ReportDhtValues() { }
        virtual void FlashLed(int times) { }
        virtual void OpenWindow() { }
        virtual void CloseWindow() { }

        virtual void WindowState(::WindowState ws) { m_windowState = ws; }
        virtual ::WindowState WindowState() const { return m_windowState; }
        virtual void AutoMode(bool am) { m_autoMode = am; }
        virtual bool AutoMode() const { return m_autoMode; }
        virtual void OpenTemp(bool ot) { m_openTemp = ot; }
        virtual int OpenTemp() const { return m_openTemp; }

    private:
        ::Log m_log;
        bool m_dhtFailSent;
        bool m_autoMode;
        int m_openTemp;
        ::WindowState m_windowState;
};
