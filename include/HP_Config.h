#ifndef HP_CONFIG_h
#define HP_CONFIG_h

#define ON  1
#define OFF 0

/* start config */
#define __DEBUG_MODE OFF


#define HP_STR_SSID    "NSA.Host.2.4"
#define HP_STR_PWD     "Alexandru1"

#define PIN_CAM_FLASH   4
#define PIN_LED         33

#if __DEBUG_MODE == ON
    #define DBG_OUTPUT_PORT Serial
    #define LOG_Println(x)  DBG_OUTPUT_PORT.println((x))
    #define LOG_Init()      DBG_OUTPUT_PORT.begin(115200);\
                            DBG_OUTPUT_PORT.setDebugOutput(true)
#else
    #define LOG_Println(x)
    #define LOG_Init()
#endif


#define HP_CMD_SLOTS        3u
#define HP_PRELOAD_CMD_NUM  10

#endif