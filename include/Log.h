#ifndef LOG_h
#define LOG_h

#include "TelnetService.h"

#if ENABLE_DEBUG == ON
    #if ON == ENABLE_TELNET
    #define DBG_OUTPUT_PORT             telnetService
    #define hp_log_init()               DBG_OUTPUT_PORT.begin(23)
    #else
    #define DBG_OUTPUT_PORT             Serial
    #define hp_log_init()               DBG_OUTPUT_PORT.begin(115200)
    #endif
    #define hp_log_printf(format, ...)  DBG_OUTPUT_PORT.printf(format, ##__VA_ARGS__)
#else
    #define hp_log_init()
    #define hp_log_printf(foramt, ...)
#endif

#endif /* LOG_h */