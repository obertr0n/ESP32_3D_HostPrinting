#ifndef OTA_HANDLER_h
#define OTA_HANDLER_h

#include "ESPAsyncWebServer.h"

extern void ota_init(AsyncWebServer& server);
extern void ota_loop();

#endif