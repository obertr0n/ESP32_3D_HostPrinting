
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include "HP_Config.h"
#include "SD_Handler.h"
#include "OTA_Handler.h"

const char *ssid = STR_SSID;
const char *password = STR_PWD;

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

static SdHandler sdHandler;

static bool b_flagRebootReq = false;

String millis2time()
{
    String Time = "";
    unsigned long ss;
    byte mm, hh;
    ss = millis() / 1000;
    hh = ss / 3600;
    mm = (ss - hh * 3600) / 60;
    ss = (ss - hh * 3600) - mm * 60;
    if (hh < 10)
        Time += "0";
    Time += (String)hh + ":";
    if (mm < 10)
        Time += "0";
    Time += (String)mm + ":";
    if (ss < 10)
        Time += "0";
    Time += (String)ss;

    return Time;
}

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

        // String payloadString = (const char *)payload;

        // LOG_Println("payload: '" + payloadString + "', channel: " + (String)len);

        // byte separator = payloadString.indexOf('=');
        // String var = payloadString.substring(0, separator);
        // String val = payloadString.substring(separator + 1);
        // if (var == "LEDonoff")
        // {
        //     LEDonoff = false;
        //     if (val == "LED = ON")
        //         LEDonoff = true;
        //     // digitalWrite(LED, HIGH);
        // }
        // else if (var == "sliderVal")
        // {
        //     sliderVal = val.toInt();
        //     LEDmillis = 9 * (100 - sliderVal) + 100;
        // }
    }
}

void setup(void)
{
    LOG_Init();
    #if 0
    pinMode(PIN_CAM_FLASH, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    #endif
    
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
            LOG_Println("index ");
            Serial.print(index);
            LOG_Println("Length ");
            Serial.print(len);

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
                  request->send(200, "text/plain", sdHandler.listDirJSON("/gcode"));
              });

    sdHandler.listDirJSON("/gcode");

    OTA_SetupSevices(server);

    ws.onEvent(webSocketEvent);
    server.addHandler(&ws);

    server.begin();
}

void loop(void)
{
#if 0
    digitalWrite(PIN_CAM_FLASH, HIGH);
    delay(500);
    digitalWrite(PIN_CAM_FLASH, LOW);
    delay(100);
    digitalWrite(PIN_LED, HIGH);
    delay(500);
    digitalWrite(PIN_LED, LOW);
    delay(100);
#endif
    //     ws.textAll(JSONtxt);
}