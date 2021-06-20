#pragma once

#include "Greenhouse.h"

#include "ArduinoLog.h"
#include "BlynkCommon.h"

#include <Arduino.h>
#include <DHT.h>

const int unknown = -1;

class GreenhouseArduino : public Greenhouse {
    public:
        static GreenhouseArduino& Instance();
        static void Instance(GreenhouseArduino& ga);

        GreenhouseArduino();

        void Setup();
        void Loop();

        const ::Log& Log() const { return m_log; }

        void HandleAutoMode(bool autoMode);
        void HandleOpenTemp(bool openTemp);
        void HandleRefreshRate(int refreshRate);
        void HandleWindowOpen(int openWindow);
        void HandleReset(int reset);
        void HandleRefresh(int refresh);

    protected:
        bool ReadDhtSensor();
        float Temperature() const;
        float Humidity() const;
        void ReportDhtValues();
        void FlashLed(int times);
        void OpenWindow();
        void CloseWindow();
        void Reboot();

    private:
        ArduinoLog m_log;
        float m_temperature;
        float m_humidity;
        int m_timerId = unknown;
        int m_led = LOW;
        DHT m_dht;
        BlynkTimer m_timer;
};
