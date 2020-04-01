#include <SPIFFS.h>
#include <Update.h>

#include "OTA_Handler.h"
#include "HP_Config.h"

void OTA_SetupSevices(AsyncWebServer& server)
{
    server.on("/fwupload", HTTP_GET,
               [&](AsyncWebServerRequest *request) {
                   LOG_Println("req /fwupload");
                   request->send(SPIFFS, "/www/otaupload.html", "text/html");
               });

    server.on("/fwupload", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            int code = 200;
            String content = "Upload OK. Reboot the board";
            AsyncWebServerResponse *response;
            
            if(Update.hasError())
            {
                code = 500;
                content = "Upload Failed.";
            }
            
            response = request->beginResponse(code, "text/plain", content);
            response->addHeader("Connection", "close");
            response->addHeader("Access-Control-Allow-Origin", "*");

            request->send(response);
        },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            /* first chunk of data */
            if (!index)
            {
                /* decide which section should be used */
                int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
                /* start with MAX size */
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
                {
                    LOG_Println(Update.errorString());
                }
            }
            /* write to flash */
            if (Update.write(data, len) != len)
            {
                LOG_Println(Update.errorString());
            }
            /* last frame of the data */
            if (final)
            {
                Update.end(true);
            }
        });
}