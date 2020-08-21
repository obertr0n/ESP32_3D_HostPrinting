#ifndef LOG_h
#define LOG_h

#include "Telnet_Server.h"

/* section misc */
#if __DEBUG_MODE == ON
    #define DBG_OUTPUT_PORT     Serial
    #define LOG_Init()          DBG_OUTPUT_PORT.begin(115200)
    #define LOG_Println(x)      DBG_OUTPUT_PORT.println((x))
#else
    #define LOG_Println(x)
    #define LOG_Init()
#endif

#endif /* LOG_h */