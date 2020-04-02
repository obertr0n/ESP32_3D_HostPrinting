#ifndef OTA_HANDLER_h
#define OTA_HANDLER_h

#include "ESPAsyncWebServer.h"

extern void OTA_SetupSevices(AsyncWebServer& server);
extern void OTA_Loop();

#endif