
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
    InitTask_HouseKeeping(); /* prio 4 */
    InitTask_GcodeTx(); /* prio 6 */
    InitTask_GcodeRx(); /* prio 5 */
}

void loop(void)
{

}