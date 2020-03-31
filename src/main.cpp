
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <SPIFFS.h>

#include "HP_Config.h"
#include "SdHandler.h"

const char *ssid = STR_SSID;
const char *password = STR_PWD;

int LED = 4;            // PINnumber where your LED is
int websockMillis = 50; // SocketVariables are sent to client every 50 milliseconds
int sliderVal = 60;     // Default value of the slider

File uploadFile;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

SdHandler sdHandler;

String JSONtxt;
unsigned long websockCount = 0UL, wait000 = 0UL, wait001 = 0UL;
int LEDmillis = 9 * (100 - sliderVal) + 100;
boolean LEDonoff = true;

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

void setupOTA(AsyncWebServer *server)
{
    server->on("/fwupdate", HTTP_GET, [&](AsyncWebServerRequest *request){
        LOG_Println("req /update");
        request->send(SPIFFS, "/www/otaauth.html", "text/html");
            });

    server->on("/www/style.css", HTTP_GET, [&](AsyncWebServerRequest *request){
        LOG_Println("req style.css");
        request->send(SPIFFS, "/www/style.css", "text/css");
            });

    server->on("/fwupload",
        HTTP_GET, 
        [&](AsyncWebServerRequest *request){
        LOG_Println("req /fwupload");
        request->send(SPIFFS, "/www/otaupload.html", "text/html");
            });

    server->on("/fwupload", 
        HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            // the request handler is triggered after the upload has finished...
            // create the response, add header, and send response
            LOG_Println("req POST /fwupload");
            AsyncWebServerResponse *response = request->beginResponse((Update.hasError()) ? 500 : 200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            response->addHeader("Connection", "close");
            response->addHeader("Access-Control-Allow-Origin", "*");

            request->send(response);
        }, 
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            //Upload handler chunks in data
            if (!index)
            {
                int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
                { 
                    // Start with max available size
                    Update.printError(Serial);
                }
            }

            // Write chunked data to the free sketch space
            if (Update.write(data, len) != len)
            {
                Update.printError(Serial);
            }

            if (final)
            { 
                // if the final flag is set then this is the last frame of data
                if (Update.end(true))
                { 
                    //true to set the size to the current progress
                }
            }
        });
}

void setup(void)
{
    DBG_OUTPUT_PORT.begin(115200);
    DBG_OUTPUT_PORT.setDebugOutput(true);
    LOG_Println("\n");

    while(!sdHandler.begin())
    {
        LOG_Println("Failed to init SD");
    }
    if(!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
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

    // WiFi.softAP("TestAP", "");
    // LOG_Println(WiFi.softAPIP());
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/www/index.html", "text/html");
    });

    server.on("/", 
        HTTP_POST, 
        [&](AsyncWebServerRequest *request) {
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

    setupOTA(&server);

    sdHandler.listDirJSON("/gcode");
    ws.onEvent(webSocketEvent);
    server.addHandler(&ws);

    server.begin();
}

void loop(void)
{
    //     ws.textAll(JSONtxt);
}