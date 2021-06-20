#include "Greenhouse.h"
#include "SerialLog.h"

class GreenhouseArduino : public Greenhouse {
    public:
        static GreenhouseArduino& Instance();
        static void Instance(GreenhouseArduino& ga);

        void Setup();
        void Loop();

    protected:
        const Log& Log() const { return m_log; }
        float GetTemperature() const;
        float GetHumidity() const;
        const bool IsDhtReady() const;
        void ReportTemperature(float t);
        void ReportHumidity(float f);
        void FlashLed(int times);
        void OpenWindow() { }
        void CloseWindow() { }

    private:
        SerialLog m_log;
};
