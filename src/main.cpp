
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
    LOG_Init();

    /* init utilities */
    Util.begin();

    /* setup the WiFi connection */
    /* either connect to an already saved network create a portal for setting it up */
    WiFiManager.begin();

    /* init file handler */
    FileHandler.begin();

    /* async webserver */
    PrintServer.begin();

    /* print handler */
    PrintHandler.begin(&PRINTER_SERIAL);

    /* telnet service */
    // TelnetLog.begin();

    LOG_Println("Init Done");
    /* signal successful init */
    Util.blinkStatus();
}

void loop(void)
{
    // TelnetLog.loop();

    PrintHandler.loopRx();
    PrintHandler.loopTx();

    /* mainly OTA checks */
    PrintServer.loop();
}