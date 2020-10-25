#include <Update.h>

#include "WebPrintServer.h"
#include "PrintHandler.h"
#include "GcodeHost.h"
#include "FileSysHandler.h"
#include "WiFiManager.h"
#include "Util.h"

using namespace std;

WebPrintServer webServer;

void WebPrintServer::begin()
{
    hp_log_printf("WebPrintServer starting...\n");
    /* websocket handler config */
    _webSocket->onEvent(bind(&WebPrintServer::webSocketEvent, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5, placeholders::_6));
    _webServer->addHandler(_webSocket);

    /* serve css file */
    _webServer->on("/www/style.css", HTTP_GET, bind(&WebPrintServer::webServerGETLoadCSS, this, placeholders::_1));
    /* serve html index */
    _webServer->on("/", HTTP_GET, bind(&WebPrintServer::webServerDefault, this, placeholders::_1));
    /* handle file upload */
    _webServer->on("/", HTTP_POST, bind(&WebPrintServer::webServerDefault, this, placeholders::_1),
            bind(&WebPrintServer::webServerPOSTUploadFile, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5, placeholders::_6));
    /* handle PrintHandler request */
    _webServer->on("/request", HTTP_ANY, bind(&WebPrintServer::webServerANYGcodeRequest, this, placeholders::_1));
    /* handle directory listing */
    _webServer->on("/dirs", HTTP_GET, bind(&WebPrintServer::webServerGETListDirectories, this, placeholders::_1));
    /* handle AbortPrint requests */
    _webServer->on("/abortPrint", HTTP_GET, bind(&WebPrintServer::webServerGETAbortPrint, this, placeholders::_1));
    /* firmware upload --> handle file upload */
    _webServer->on("/fwupload", HTTP_POST, bind(&WebPrintServer::webServerDefault, this, placeholders::_1),
            bind(&WebPrintServer::webServerPOSTUploadFirmware, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5, placeholders::_6));
    _webServer->on("/resetWifi", HTTP_GET, bind(&WebPrintServer::webServerGETResetWifiSettings, this, placeholders::_1));
    
    hp_log_printf("WebServer begin...\n");
    /* start our async webServer */
    _webServer->begin();
    
    hp_log_printf("WebServer started\n");
}

void WebPrintServer::loop()
{
    if (WebPrintServer::_rebootRequired)
    {
        Util.sysReboot();
    }
}

void WebPrintServer::write(String& text)
{
    if (_webSocket->count() > 0 && _webSocket->availableForWriteAll())
    {
        _webSocket->textAll(text);
    }
}

void WebPrintServer::webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *payload, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        hp_log_printf("Websocket client connection received\n");
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        hp_log_printf("Client disconnected\n");
    }
    else if (type == WS_EVT_DATA)
    {
        char data[124];
        memcpy(data, payload, 124);
        data[123] = '\0';

        hp_log_printf("payload: %s\nlen: %d\n", data, len);
    }
}

void WebPrintServer::webServerGETLoadCSS(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/www/style.css.gz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void WebPrintServer::webServerDefault(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/www/index.html.gz", "text/html");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

void WebPrintServer::webServerPOSTUploadFile(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (printHandler.isPrinting())
    {
        request->send(403, "text/plain", "Job not accepted");
    }
    else
    {
        /* first chunk, index is the byte index in the data, len is the current chunk length sent */
        if (!index)
        {
            String filePath = "/gcode/" + filename;
            request->_tempFile = fileHandler.openFile(filePath, FILE_WRITE);
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
}

void WebPrintServer::webServerANYGcodeRequest(AsyncWebServerRequest *request)
{
    int code = 403;
    bool res = false;
    String text;
    if (request->hasArg("printFile"))
    {
        text = request->arg("printFile");
        if (!printHandler.isPrinting())
        {
            if(printHandler.requestPrint(text))
            {
                code = 200;
                text = "OK";
            }
            else
            {
                text = "Job not accepted";
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
        if(!text.isEmpty())
        {
            res = gcodeHost.addCommand(text, false, false);
        }
    }
    else if (request->hasArg("masterCmd"))
    {
        text = request->arg("masterCmd");
        /* a master command can be send even during print! */
        res = gcodeHost.addCommand(text, true, false);
    }
    else if (request->hasArg("deletef"))
    {
        text = request->arg("deletef");
        res = fileHandler.remove(text);
    }
    else
    {
        text = "Job not accepted";
    }

    if (res)
    {
        code = 200;
        text = "OK";
    }
    else
    {
        text = "Operation Failed";
    }
    request->send(code, "text/plain", text);
}

void WebPrintServer::webServerGETListDirectories(AsyncWebServerRequest *request)
{
    if (printHandler.isPrinting())
    {
        request->send(403, "text/plain", "Job not accepted");
    }
    else
    {
        request->send(200, "text/plain", fileHandler.jsonifyDir(".gcode"));
    }
}

void WebPrintServer::webServerGETAbortPrint(AsyncWebServerRequest *request)
{
    if (!printHandler.isPrinting())
    {
        request->send(403, "text/plain", "Job not accepted");
    }
    else
    {
        printHandler.abortPrint();
        request->send(200, "text/plain", "Print job Cancelled!");
    }
}

void WebPrintServer::webServerGETResetWifiSettings(AsyncWebServerRequest *request)
{
    wifiManager.resetSetting();
    request->send(200, "text/plain", "Ok");
}

void WebPrintServer::webServerPOSTUploadFirmware(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        /* fresh start */
        Update.abort();
        /* decide which section should be used */
        WebPrintServer::_uploadType = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
        /* start with MAX size */
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, WebPrintServer::_uploadType))
        {
            hp_log_printf("%s\n", Update.errorString());
        }
    }
    /* write to flash */
    if (Update.write(data, len) != len)
    {
        hp_log_printf("%s\n", Update.errorString());
    }
    /* last frame of the data */
    if (final)
    {
        if(UPDATE_ERROR_OK == Update.getError())
        {
            /* reboot only if program flash upload */
            if (Update.end(true) && (WebPrintServer::_uploadType == U_FLASH))
            {
                WebPrintServer::_rebootRequired = true;
            }
        }
        else
        {
            hp_log_printf("%s\n", Update.errorString());
        }
    }
}
