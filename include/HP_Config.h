#ifndef HP_CONFIG
#define HP_CONFIG

/* start config */

#define __DEBUG_MODE ON


#define STR_SSID    "NSA.Host.2.4"
#define STR_PWD     "Alexandru1"



#if __DEBUG_MODE == ON
    #define DBG_OUTPUT_PORT Serial
    #define LOG_Println(x)  DBG_OUTPUT_PORT.println((x))
#else
    #define LOG_Println(x)
#endif

#endif