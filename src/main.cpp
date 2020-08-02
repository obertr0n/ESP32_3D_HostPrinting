
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include "HP_Config.h"
#include "FS_Handler.h"
#include "OTA_Handler.h"
#include "Print_Handler.h"
#include "HP_Util.h"

const char *ssid = HP_STR_SSID;
const char *password = HP_STR_PWD;

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static FSHandler fsHandler;
static PrintHandler printHandler(&Serial);

static File printFile;
static TaskHandle_t ph_Txtask_handle = NULL;
static TaskHandle_t ph_Rxtask_handle = NULL;

void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *payload, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        LOG_Println("Websocket client connection received");
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        LOG_Println("Client disconnected");
    }
    else if (type == WS_EVT_DATA)
    {
        char data [124];
        memcpy(data, payload, len);
        data[len] = '\0';

        LOG_Println("payload: " + (String)data + ", len: " + (String)len);
    }
}

// void printTx_service_task(void *pvParameters)
// {
//     for(;;)
//     {
//         /* printing powerhouse :D */
//         printHandler.loopTx();
//     }
//     vTaskDelete(NULL);
//     ph_Txtask_handle = NULL;
// }

// void printRx_service_task(void *pvParameters)
// {
//     for(;;)
//     {
//         /* printing powerhouse :D */
//         printHandler.loopRx();
//     }
//     vTaskDelete(NULL);
//     ph_Txtask_handle = NULL;
// }

void setup_ph_task()
{
    /* our print handler, maybe move it to the other core */
    printHandler.begin(&ws);

    // xTaskCreateUniversal(printTx_service_task, 
    //                     "ph_Txtask", 
    //                     8192 * 2, 
    //                     NULL, 
    //                     1, 
    //                     &ph_Txtask_handle, 
    //                     0);

    // xTaskCreateUniversal(printRx_service_task, 
    //                     "ph_Rxtask", 
    //                     8192 * 2, 
    //                     NULL, 
    //                     1, 
    //                     &ph_Rxtask_handle, 
    //                     0);
}

void setup(void)
{
    LOG_Init();
    
    // disableCore0WDT();
    // disableCore1WDT();

    while (!fsHandler.begin())
    {
        LOG_Println("Failed to init FS");
    }

    // WiFi.softAP("ESP_TestAP");
    // Serial.println(WiFi.softAPIP().toString());

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
    
    /* init OTA services */
    ota_init(server);
    
    /* websocket handler config */
    ws.onEvent(webSocketEvent);
    server.addHandler(&ws);
    
    /* setup PrintHandler task */
    setup_ph_task();

    /* serve css file */
    server.on("/www/style.css", HTTP_GET, [&](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/www/style.css", "text/css");
    });
    /* serve html file */
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/www/index.html", "text/html");
    });
    /* file upload */
    server.on("/", HTTP_POST, [&](AsyncWebServerRequest *request) {
            /* this will get called after the req is completed */
            request->send(SPIFFS, "/www/index.html", "text/html");
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  
        if (printHandler.isPrinting())
        {
            request->send(403, "text/plain", "Job not acccepted");
        }
        else
        { 
            /* first chunk, index is the byte index in the data, len is the current chunk length sent */
            if (!index)
            {
                String filePath = "/gcode/" + filename;

                LOG_Println(filePath);
                request->_tempFile = fsHandler.openFile(filePath, FILE_WRITE);
            }

            /* write the received bytes */
            request->_tempFile.write(data, len);

            /* final chunk */
            if (final)
            {
                /* close the file */
                request->_tempFile.close();
            }
        }
    });
    /* gcode commands */
    server.on("/request", HTTP_ANY, [](AsyncWebServerRequest *request) {
        int code = 403;
        bool res = false; 
        String text;
        if (request->hasArg("printFile"))
        {
            text = request->arg("printFile");

            if (fsHandler.exists(text))
            {
                if (!printHandler.isPrinting())
                {
                    printFile = fsHandler.openFile(text, FILE_READ);
                    printHandler.requestPrint(printFile);
                    code = 200;
                    text = "OK";
                }
                else
                {
                    text = "Job not acccepted";
                }
            }
            else
            {
                text = "File not found!";
            }
        }
        else if (request->hasArg("gcodecmd"))
        {
            text = request->arg("gcodecmd");
            res = printHandler.queueCommand(text, false, false);
        }
        else if(request->hasArg("masterCmd"))
        {
            text = request->arg("masterCmd");
            /* a master command can be send even during print! */
            res = printHandler.queueCommand(text, true, false);
        }
        else if(request->hasArg("deletef"))
        {
            text = request->arg("deletef");
            res = fsHandler.remove(text);
        }
        else
        {
            text = "Job not acccepted";
        }

        if(res)
        {
            code = 200;
            text = "OK";
        }
        else
        {
            text = "Operation Failed";
        }

        request->send(code, "text/plain", text);
    });
    /* list directories */
    server.on("/dirs", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (printHandler.isPrinting())
        {
            request->send(403, "text/plain", "Job not acccepted");
        }
        else
        {
            request->send(200, "text/plain", fsHandler.jsonifyDir(".gcode"));
        }
    });
    /* abort a print job */
    server.on("/abortPrint", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!printHandler.isPrinting())
        {
            request->send(403, "text/plain", "Job not acccepted");
        }
        else
        {
            printHandler.abortPrint();
            request->send(200, "text/plain", "Print job Cancelled!");
        }
    });
    /* start our async server */
    server.begin();
    
    LOG_Println("Init Done");
    /* signal successful init */
    util_blink_status();
}

void loop(void)
{
    /* check if a reboot is required */
    ota_loop();
        
    printHandler.loopRx();
    printHandler.loopTx();

}