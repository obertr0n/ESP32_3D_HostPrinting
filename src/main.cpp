
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include "HP_Config.h"
#include "SD_Handler.h"
#include "OTA_Handler.h"
#include "Print_Handler.h"
#include "HP_Util.h"

const char *ssid = HP_STR_SSID;
const char *password = HP_STR_PWD;

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static SdHandler sdHandler;
static PrintHandler printHandler(&Serial);

static String printFileName;
static bool printRequested = false;
static File printFile;

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
        char filename [124];
        memcpy(filename, payload, len);
        filename[len] = '\0';
        printFileName = (String) filename;
        printRequested = true;

        LOG_Println("payload: '" + printFileName + "', len: " + (String)len);
    }
}

void setup(void)
{
    LOG_Init();
    
    while (!sdHandler.begin())
    {
        LOG_Println("Failed to init SD");
    }
    if (!SPIFFS.begin())
    {
        LOG_Println("An Error has occurred while mounting SPIFFS");
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

    server.on("/www/style.css", HTTP_GET, [&](AsyncWebServerRequest *request) {
        LOG_Println("req style.css");
        request->send(SPIFFS, "/www/style.css", "text/css");
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/www/index.html", "text/html");
    });

    server.on("/", HTTP_POST, [&](AsyncWebServerRequest *request) {
            LOG_Println("req POST /");

            request->send(SPIFFS, "/www/index.html", "text/html");
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  
            String filePath = "/gcode/" + filename;
            /* first chunk, index is the byte index in the data, len is the current chunk length sent */
            if (!index)
            {
                sdHandler.remove(filePath);
                request->_tempFile = sdHandler.openFile(filePath, FILE_WRITE);
            }

            /* write the received bytes */
            request->_tempFile.write(data, len);

            /* final chunk */
            if (final)
            {
                /* close the file */
                request->_tempFile.close();
            }
        });

    server.on("/dirs",
              HTTP_GET,
              [](AsyncWebServerRequest *request) {
                  LOG_Println("req /dirs");
                  request->send(200, "text/plain", sdHandler.jsonifyDir("/gcode", ".gcode"));
              }); 
    
    /* init utilities */
    util_init();
    /* inir OTA services */
    ota_init(server);
    /* websocket handler config */
    ws.onEvent(webSocketEvent);
    server.addHandler(&ws);
    /* start our async server */
    server.begin();
    /* our print handler, maybe move it to the other core */
    printHandler.begin(115200);
    /* signal successful init */
    util_blink_status();
}

void loop(void)
{
    /* telnet support */
    // util_telnetLoop();

    if(printRequested)
    {            
        printRequested = false;
        
        printFile = sdHandler.openFile(printFileName, "r");
        printHandler.startPrint(printFile);
    }

    /* check if a reboot is required */
    ota_loop();

    /* printing powerhouse :D */
    printHandler.loop();

    // util_telnetSend("This is a test" + i);
    //     ws.textAll(JSONtxt);
}