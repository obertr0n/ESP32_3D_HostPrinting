#ifndef WEB_PRINT_SERVER_h
#define WEB_PRINT_SERVER_h


#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include "Config.h"

class WebPrintServer
{
    private:
        AsyncWebServer* _webServer;
        AsyncWebSocket* _webSocket;
        File _printFile;
        const String _uri = "/ws";
        bool _rebootRequired;
        int _uploadType;

        void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *payload, size_t len);
        void webServerGETLoadCSS(AsyncWebServerRequest *request);
        void webServerDefault(AsyncWebServerRequest *request);
        void webServerPOSTUploadFile(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
        void webServerANYGcodeRequest(AsyncWebServerRequest *request);
        void webServerGETListDirectories(AsyncWebServerRequest *request);
        void webServerGETAbortPrint(AsyncWebServerRequest *request);
        void webServerPOSTUploadFirmware(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
    public:
        WebPrintServer()
        {
            _webSocket = new AsyncWebSocket(_uri);
            _webServer = new AsyncWebServer(80);

            _uploadType = -1;
            _rebootRequired = false;            
        };
        ~WebPrintServer()
        {
            _webSocket->closeAll();
            _webServer->removeHandler(_webSocket);
            _webServer->end();

            delete _webSocket;
            delete _webServer;
        }
        void write(String& text);
        void begin();
        void loop();
};

extern WebPrintServer PrintServer;

#endif /* WEB_PRINT_SERVER_h */