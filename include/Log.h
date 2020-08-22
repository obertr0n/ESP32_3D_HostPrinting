#ifndef LOG_h
#define LOG_h

#include "Telnet_Server.h"
/* section misc */
#if __DEBUG_MODE == ON
    #if ON == USE_TELNET
    #define DBG_OUTPUT_PORT     TelnetLog
    #define LOG_Init()          DBG_OUTPUT_PORT.begin(23)
    #else
    #define DBG_OUTPUT_PORT     Serial
    #define LOG_Init()          DBG_OUTPUT_PORT.begin(115200)
    #endif
    #define LOG_Println(x)      DBG_OUTPUT_PORT.println((x))
#else
    #define LOG_Println(x)
    #define LOG_Init()
#endif

#endif /* LOG_h */