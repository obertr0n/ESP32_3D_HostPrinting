
#include <WiFi.h>

#include <AsyncTCP.h>
#include <DNSServer.h>
#include <vector>

#include "HP_Config.h"
#include "FS_Handler.h"
#include "Print_Handler.h"
#include "WebPrint_Server.h"
#include "Telnet_Server.h"
#include "HP_Util.h"

const char *ssid = HP_STR_SSID;
const char *password = HP_STR_PWD;

void setup(void)
{
    // LOG_Init();

    while (!FileHandler.begin())
    {
        LOG_Println("Failed to init FS");
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    LOG_Println("Connecting to ");
    LOG_Println(ssid);

    while (WiFi.status() != WL_CONNECTED)
    {
        LOG_Println(".");
        delay(300);
    }

    LOG_Println("Connected! IP address: ");
    LOG_Println(WiFi.localIP());

    /* init utilities */
    util_init();    
    
    /* async webserver */
    PrintServer.begin();

    /* print handler */
    HP_Handler.begin(&PRINTER_SERIAL);
    
    /* telnet service */
    TelnetLog.begin();
    
    LOG_Println("Init Done");
    /* signal successful init */
    util_blink_status();
}

void loop(void)
{        
    HP_Handler.loopRx();
    HP_Handler.loopTx();

    /* mainly OTA checks */
    PrintServer.loop();

    TelnetLog.loop();    
}