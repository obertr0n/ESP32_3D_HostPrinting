
#include <WiFi.h>

#include <AsyncTCP.h>
#include <DNSServer.h>
#include <vector>

#include "Config.h"
#include "FileSys_Handler.h"
#include "Print_Handler.h"
#include "WebPrint_Server.h"
#include "WiFi_Manager.h"
#include "Telnet_Server.h"
#include "Util.h"
#include "Log.h"

void setup(void)
{
#if __DEBUG_MODE == ON && USE_TELNET == ON
    /* setup the WiFi connection */
    /* either connect to an already saved network or create a portal for setting it up */
    WiFiManager.begin();
    
    /* test like this */
    LOG_Init();
#else    
    /* test like this */
    LOG_Init();
    /* setup the WiFi connection */
    /* either connect to an already saved network or create a portal for setting it up */
    WiFiManager.begin();
#endif

    /* init utilities */
    Util.begin();

    /* init file handler */
    FileHandler.begin();

    /* async webserver */
    PrintServer.begin();

    /* print handler */
    PrintHandler.begin(&PRINTER_SERIAL);

#if __DEBUG_MODE == OFF && USE_TELNET == ON
    /* telnet service */
    TelnetLog.begin(23);
#endif

    LOG_Println("Init Done");
    /* signal successful init */
    Util.blinkSuccess();
}

void loop(void)
{
    PrintHandler.loopRx();
    PrintHandler.loopTx();
    /* mainly OTA checks */
    PrintServer.loop();
#if ON == USE_TELNET 
    TelnetLog.loop();
#endif
}