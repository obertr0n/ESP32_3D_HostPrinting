
#include "WiFiManager.h"
#include "FileSysHandler.h"
#include "PrintHandler.h"
#include "WebPrintServer.h"
#include "SerialHandler.h"
#include "GcodeHost.h"
#if ON == ENABLE_TELNET
#include "TelnetService.h"
#endif
#include "Util.h"
#include "Log.h"
#include "Task.h"
#include "Config.h"

void setup(void)
{
#if ENABLE_DEBUG == ON && ENABLE_TELNET == ON
    /* setup the WiFi connection */
    /* either connect to an already saved network or create a portal for setting it up */
    wifiManager.begin();
    
    /* test like this */
    hp_log_init();
#else    
    /* test like this */
    hp_log_init();
    /* setup the WiFi connection */
    /* either connect to an already saved network or create a portal for setting it up */
    wifiManager.begin();
#endif

    /* init utilities */
    Util.begin();

    /* init file handler */
    fileHandler.begin();

    /* async webserver */
    webServer.begin();
    /* our serial handler */
    serialHandler.begin();
    /* print handler */
    printHandler.begin();
    /* gcode host handler */
    gcodeHost.begin();

#if ENABLE_DEBUG == OFF && ENABLE_TELNET == ON
    /* telnet service */
    TelnetLog.begin(23);
#endif

    hp_log_printf("Init Done\n");
    /* signal successful init */
    Util.blinkSuccess();

    /* configure tasks */
    // LoggingTask_init();
    PrintStateTask_init();    
    // gcodeHost.attemptConnect();
    GcodeTxTask_init();
    GcodeRxTask_init();
}
// static uint32_t timeout = 0; 
void loop(void)
{
    // if(timeout < millis())
    // {
    //     hp_log_printf("Heap total: %d\n", ESP.getHeapSize());
    //     hp_log_printf("Worst: %d\n", ESP.getMinFreeHeap());
    //     hp_log_printf("Biggest block %d\n", ESP.getMaxAllocHeap());
    //     hp_log_printf("Heap: %d\n", Util.getHeapUsedPercent());
    //     timeout += 1000;
    // }
}