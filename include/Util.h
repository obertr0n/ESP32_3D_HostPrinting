#ifndef UTIL_h
#define UTIL_h

#include <Esp.h>
#include <WiFi.h>
#include "Config.h"

class UtilClass
{
    public:
        void begin()
        {
            pinMode(PIN_CAM_FLASH, OUTPUT);
            digitalWrite(PIN_CAM_FLASH, LOW);
            pinMode(PIN_LED, OUTPUT);
        };

        String millisToTime();

        String getIp()
        {
            if(WiFi.getMode() == WIFI_MODE_STA)
            {
                if(WL_CONNECTED == WiFi.status())
                {
                    return WiFi.localIP().toString();
                }
            }
            else if(WiFi.getMode() == WIFI_MODE_AP)
            {
                return WiFi.softAPIP().toString();
            }

            return "";
        };

        // String getChipId()
        // {

        // };

        String getMac()
        {
            char macValue[17]; // Don't forget one byte for the terminating NULL...
            uint64_t mac = ESP.getEfuseMac();
            (void)sprintf(macValue, "%016llx", mac);

            return String(macValue);
        };

        uint32_t getHeapUsedPercent()
        {
            uint32_t used = ESP.getHeapSize() - ESP.getFreeHeap();
            return 100 * used / ESP.getHeapSize();
        };

        void blinkStatus()
        {
            for(int i = 0; i < 5; i++)
            {
                digitalWrite(PIN_LED, LOW);
                delay(200);
                digitalWrite(PIN_LED, HIGH);
                delay(300);
            }
        };
    private:
};

extern UtilClass Util;

#endif