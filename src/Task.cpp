
#include "esp_task_wdt.h"

#include "Task.h"
#include "Util.h"
#include "GcodeHost.h"
#include "WebPrintServer.h"
#include "TelnetService.h"
#include "PrintHandler.h"

#define GRX_TASK_PRIO   5
#define GRX_TASK_CORE   1

#define GTX_TASK_PRIO   6
#define GTX_TASK_CORE   1

#define HC_TASK_PRIO    4
#define HC_TASK_CORE    1

TaskHandle_t _gcodeRxTH = NULL;
TaskHandle_t _gcodeTxTH = NULL;

TaskHandle_t _houseKeepingTH = NULL;

static void GcodeRx_process(void* param);
static void GcodeTx_process(void* param);

static void HouseKeeping_process(void* param);

void InitTask_GcodeRx()
{
    if (_gcodeRxTH == NULL) 
    {
        xTaskCreatePinnedToCore(
            GcodeRx_process, /* Task function. */
            "rtx_task", /* name of task. */
            2048, /* Stack size of task */
            NULL, /* parameter of the task */
            GRX_TASK_PRIO, /* priority of the task */
            &_gcodeRxTH, /* Task handle to keep track of created task */
            GRX_TASK_CORE    /* Core to run the task */
        );
    }
}

void InitTask_GcodeTx()
{
    if (_gcodeTxTH == NULL) 
    {
        xTaskCreatePinnedToCore(
            GcodeTx_process, /* Task function. */
            "gtx_task", /* name of task. */
            2048, /* Stack size of task */
            NULL, /* parameter of the task */
            GTX_TASK_PRIO, /* priority of the task */
            &_gcodeTxTH, /* Task handle to keep track of created task */
            GTX_TASK_CORE    /* Core to run the task */
        );
    }
}

void InitTask_HouseKeeping()
{
    if (_houseKeepingTH == NULL) 
    {
        xTaskCreatePinnedToCore(
            HouseKeeping_process, /* Task function. */
            "hc_task", /* name of task. */
            2048, /* Stack size of task */
            NULL, /* parameter of the task */
            HC_TASK_PRIO, /* priority of the task */
            &_houseKeepingTH, /* Task handle to keep track of created task */
            HC_TASK_CORE    /* Core to run the task */
        );
    }
}

/* task handler */
static void GcodeRx_process(void* param)
{
    for(;;)
    {
        if(gcodeHost.loopRx())
        {
            xTaskNotifyGive(_gcodeTxTH);
        }
    }
    esp_task_wdt_delete(_gcodeRxTH);
    vTaskDelete(_gcodeRxTH);
}

static void GcodeTx_process(void* param)
{
    const TickType_t xMaxExpectedBlockTime = 50 / portTICK_PERIOD_MS;
    for(;;)
    {
        ulTaskNotifyTake(pdTRUE, xMaxExpectedBlockTime);
        gcodeHost.loopTx();
    }
    esp_task_wdt_delete(_gcodeTxTH);
    vTaskDelete(_gcodeTxTH);
}

static void HouseKeeping_process(void* param)
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
    esp_task_wdt_delete(_houseKeepingTH);
    vTaskDelete(_houseKeepingTH);
}