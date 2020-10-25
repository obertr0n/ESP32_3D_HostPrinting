
#include "esp_task_wdt.h"

#include "Task.h"
#include "Util.h"
#include "GcodeHost.h"
#include "WebPrintServer.h"
#include "TelnetService.h"
#include "PrintHandler.h"

#define GCODE_RX_TASK_PRIO             5
#define GCODE_RX_TASK_CORE             1

#define GCODE_TX_TASK_PRIO             6
#define GCODE_TX_TASK_CORE             1

#define PRINT_STATE_TASK_PRIO          4
#define PRINT_STATE_TASK_CORE          1

#define LOGGING_TASK_PRIO              7
#define LOGGING_TASK_CORE              1

TaskHandle_t gcodeRxTaskHandle = NULL;
TaskHandle_t gcodeTxTaskHandle = NULL;

TaskHandle_t loggingTaskHandle = NULL;
TaskHandle_t printStateTaskHandle = NULL;

static void GcodeRxTask_process(void* param);
static void GcodeTxTask_process(void* param);

static void LoggingTask_process(void* param);
static void PrintStateTask_process(void* param);

void GcodeRxTask_init()
{
    if (gcodeRxTaskHandle == NULL) 
    {
        xTaskCreatePinnedToCore(
            GcodeRxTask_process, /* Task function. */
            "GcodeRxProcessor", /* name of task. */
            2048, /* Stack size of task */
            NULL, /* parameter of the task */
            GCODE_RX_TASK_PRIO, /* priority of the task */
            &gcodeRxTaskHandle, /* Task handle to keep track of created task */
            GCODE_RX_TASK_CORE    /* Core to run the task */
        );
    }
}

void GcodeTxTask_init()
{
    if (gcodeTxTaskHandle == NULL) 
    {
        xTaskCreatePinnedToCore(
            GcodeTxTask_process, /* Task function. */
            "GcodeTxProcessor", /* name of task. */
            2048, /* Stack size of task */
            NULL, /* parameter of the task */
            GCODE_TX_TASK_PRIO, /* priority of the task */
            &gcodeTxTaskHandle, /* Task handle to keep track of created task */
            GCODE_TX_TASK_CORE    /* Core to run the task */
        );
    }
}

void PrintStateTask_init()
{
    if (printStateTaskHandle == NULL) 
    {
        xTaskCreatePinnedToCore(
            PrintStateTask_process, /* Task function. */
            "PrintStateProcessor", /* name of task. */
            2048, /* Stack size of task */
            NULL, /* parameter of the task */
            PRINT_STATE_TASK_PRIO, /* priority of the task */
            &printStateTaskHandle, /* Task handle to keep track of created task */
            PRINT_STATE_TASK_CORE    /* Core to run the task */
        );
    }
}

void LoggingTask_init()
{
    if (loggingTaskHandle == NULL) 
    {
        xTaskCreatePinnedToCore(
            LoggingTask_process, /* Task function. */
            "LoggingTask", /* name of task. */
            1024, /* Stack size of task */
            NULL, /* parameter of the task */
            LOGGING_TASK_PRIO, /* priority of the task */
            &loggingTaskHandle, /* Task handle to keep track of created task */
            LOGGING_TASK_CORE    /* Core to run the task */
        );
    }
}

/* task handler */
static void GcodeRxTask_process(void* param)
{
    for(;;)
    {
        if(gcodeHost.loopRx())
        {
            xTaskNotifyGive(gcodeTxTaskHandle);
        }
    }
    esp_task_wdt_delete(gcodeRxTaskHandle);
    vTaskDelete(gcodeRxTaskHandle);
}

static void GcodeTxTask_process(void* param)
{
    const TickType_t xMaxExpectedBlockTime = 50 / portTICK_PERIOD_MS;
    for(;;)
    {
        ulTaskNotifyTake(pdTRUE, xMaxExpectedBlockTime);
        gcodeHost.loopTx();
    }
}

static void LoggingTask_process(void* param)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 300 / portTICK_PERIOD_MS;
    for(;;)
    {
        #if ON == ENABLE_TELNET 
        telnetService.loop();
        //Serial.println(".");
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        #endif        
    }
}

static void PrintStateTask_process(void* param)
{
    TickType_t xLastWakeTime;
    const TickType_t xTimeout = 500 / portTICK_PERIOD_MS;
    for(;;)
    {
        printHandler.processSM();
        printHandler.updateWSState();
#if ON == ENABLE_TELNET 
        telnetService.loop();
#endif
        /* mainly OTA checks */
        webServer.loop();

        vTaskDelayUntil(&xLastWakeTime, xTimeout);
    }
}