#ifndef WEB_PRINT_SERVER_h
#define WEB_PRINT_SERVER_h


#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include "HP_Config.h"

class WebPrintServer
{
    private:
        static AsyncWebServer _webServer;
        static AsyncWebSocket _webSocket;
        static File _printFile;
        static bool _rebootRequired;
        static int _uploadType;

        static void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *payload, size_t len);
        static void webServerGETLoadCSS(AsyncWebServerRequest *request);
        static void webServerGETDefault(AsyncWebServerRequest *request);
        static void webServerPOSTDefault(AsyncWebServerRequest *request);
        static void webServerPOSTUploadFile(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
        static void webServerANYGcodeRequest(AsyncWebServerRequest *request);
        static void webServerGETListDirectories(AsyncWebServerRequest *request);
        static void webServerGETAbortPrint(AsyncWebServerRequest *request);
        static void webServerGETFirmwareUpdate(AsyncWebServerRequest *request);
        static void webServerPOSTFirmwareUpdate(AsyncWebServerRequest *request);
        static void webServerPOSTUploadFirmware(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
    public:
        WebPrintServer()
        {
            // String uri = "/ws";
            // _webSocket = AsyncWebSocket(uri);
            // _webServer = AsyncWebServer(80);
            
        };
        ~WebPrintServer()
        {

        }
        void write(String& text);
        void begin();
        void loop();
};

extern WebPrintServer PrintServer;

#endif /* WEB_PRINT_SERVER_h */