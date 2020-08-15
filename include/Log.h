#ifndef LOG_h
#define LOG_h

/* section misc */
#if __DEBUG_MODE == ON
    #define LOG_Init()          Serial.begin(115200)
    #define DBG_OUTPUT_PORT     Serial
    #define LOG_Println(x)      DBG_OUTPUT_PORT.println((x))
#else
    #define LOG_Println(x)
    #define LOG_Init()
#endif

#endif /* LOG_h */