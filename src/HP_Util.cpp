
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

    // util_telnetInit();
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

void listTasks()
{
    static bool Run = false;
    if (!Run)
    {
        Serial.printf("\r\n\r\n\r\n\r\n\r\n\r\n");
        Run = true;
        TaskStatus_t *pxTaskStatusArray;
        volatile UBaseType_t uxArraySize, x;
        uint32_t ulTotalRunTime, ulStatsAsPercentage;

        /* Take a snapshot of the number of tasks in case it changes while this
   function is executing. */
        uxArraySize = uxTaskGetNumberOfTasks();

        /* Allocate a TaskStatus_t structure for each task.  An array could be
   allocated statically at compile time. */
        pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

        /* Generate raw status information about each task. */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray,
                                           uxArraySize,
                                           &ulTotalRunTime);

        /* For percentage calculations. */
        ulTotalRunTime /= 100UL;

        /* Avoid divide by zero errors. */
        if (ulTotalRunTime > 0)
        {
            /* For each populated position in the pxTaskStatusArray array,
         format the raw data as human readable ASCII data. */
            for (x = 0; x < uxArraySize; x++)
            {
                /* What percentage of the total run time has the task used?
            This will always be rounded down to the nearest integer.
            ulTotalRunTimeDiv100 has already been divided by 100. */
                ulStatsAsPercentage =
                    pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime;

                Serial.printf("Name: %s \r\n\tPrio: %u \r\n\tRuntime \r\n\tCtr: %u \r\n\tPercent: %u \r\n",
                              pxTaskStatusArray[x].pcTaskName,
                              pxTaskStatusArray[x].uxCurrentPriority,
                              pxTaskStatusArray[x].ulRunTimeCounter,
                              ulStatsAsPercentage);
            }
        }

        /* The array is no longer needed, free the memory it consumes. */
        vPortFree(pxTaskStatusArray);
    }
}