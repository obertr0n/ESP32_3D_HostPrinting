
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

static String printFileName;
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
    // printHandler.begin(250000, &ws);
    
    disableCore0WDT();
    disableCore1WDT();

    while (!fsHandler.begin())
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

    server.on("/request", HTTP_ANY, [](AsyncWebServerRequest *request) {
        int code = 403;
        String text;
        if (request->hasArg("filename"))
        {
            printFileName = request->arg("filename");

            if (fsHandler.exists(printFileName))
            {
                if (!printHandler.isPrinting())
                {
                    printFile = fsHandler.openFile(printFileName, FILE_READ);
                    printHandler.startPrint(printFile);
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
            if (printHandler.add(text))
            {
                code = 200;
                text = "OK";
            }
            else
            {
                text = "Job not acccepted";
            }
        }
        else if(request->hasArg("deletef"))
        {
            String fileName = request->arg("deletef");
            if (fsHandler.remove(fileName))
            {
                code = 200;
                text = "OK";
            }
            else
            {
                text = "File not found!";
            }
        }
        else
        {
            text = "Job not acccepted";
        }
        request->send(code, "text/plain", text);
    });
    
    server.on("/del", HTTP_ANY, [](AsyncWebServerRequest *request) {
        int code = 403;
        String text;
        if (request->hasArg("filename"))
        {
            printFileName = request->arg("filename");

            if (fsHandler.exists(printFileName))
            {
                printFile = fsHandler.openFile(printFileName, FILE_READ);
                printHandler.startPrint(printFile);
                code = 200;
                text = "OK";
             }
            else
            {
                code = 403;
                text = "File not found!";
            }
        }
        else if (request->hasArg("gcodecmd"))
        {
            text = request->arg("gcodecmd");
            if (printHandler.send(text))
            {
                code = 200;
                text = "OK";
            }
            else
            {
                code = 403;
                text = "Job not accepted";
            }
        }
        else
        {
            code = 403;
            text = "Job not acccepted";
        }
        request->send(code, "text/plain", text);
    });
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

    server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *request) {
        String result;
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

                result += "\r\nName:" + (String)pxTaskStatusArray[x].pcTaskName +
                          "\r\n\tPirority: " + (String)pxTaskStatusArray[x].uxCurrentPriority +
                          "\r\n\tRuntimeCtr: " + (String)pxTaskStatusArray[x].ulRunTimeCounter +
                          "\r\n\tPercentage: " + (String)ulStatsAsPercentage;
            }
        }

        ws.textAll(result);
        /* The array is no longer needed, free the memory it consumes. */
        vPortFree(pxTaskStatusArray);
    });   

    /* start our async server */
    server.begin();
    
    LOG_Println("Init Done");
    /* signal successful init */
    util_blink_status();
}

void loop(void)
{
    /* telnet support */
    // util_telnetLoop();

    /* check if a reboot is required */
    ota_loop();
        
    printHandler.loopRx();
    printHandler.loopTx();

    // listTasks();
    // util_telnetSend("This is a test" + i);
    // ws.textAll(JSONtxt);
}