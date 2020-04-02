
#include <WiFi.h>
#include "HP_Util.h"
#include "HP_Config.h"

static WiFiServer telnet(23);
static WiFiClient telnetClient;

static void util_telnetInit();

String util_millisToTime()
{
    String Time = "";
    unsigned long ss;
    byte mm, hh;
    ss = millis() / 1000;
    hh = ss / 3600;
    mm = (ss - hh * 3600) / 60;
    ss = (ss - hh * 3600) - mm * 60;
    if (hh < 10)
        Time += "0";
    Time += (String)hh + ":";
    if (mm < 10)
        Time += "0";
    Time += (String)mm + ":";
    if (ss < 10)
        Time += "0";
    Time += (String)ss;

    return Time;
}

void util_init()
{
    // pinMode(PIN_CAM_FLASH, OUTPUT);
    pinMode(PIN_LED, OUTPUT);

    util_telnetInit();
}

static void util_telnetInit()
{
    telnet.begin();
    telnet.setNoDelay(true);
}

void util_telnetLoop()
{
    if (telnet.hasClient() && (!telnetClient.connected()))
    {
        telnetClient.stop();

        telnetClient = telnet.available();
        telnetClient.flush();
    }
}

void util_telnetSend(String line) 
{
    if (telnetClient.connected())
    {
        telnetClient.println(line);
    }
}

void util_blink_status()
{
    // digitalWrite(PIN_CAM_FLASH, HIGH);
    // delay(500);
    // digitalWrite(PIN_CAM_FLASH, LOW);
    // delay(500);
    for (int i = 0; i < 5; i++)
    {
        digitalWrite(PIN_LED, LOW);
        delay(200);
        digitalWrite(PIN_LED, HIGH);
        delay(300);
    }
}