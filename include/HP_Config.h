#ifndef HP_CONFIG_h
#define HP_CONFIG_h

#define ON  1U
#define OFF 0U

/* section serial */
#define PRINTER_SERIAL Serial

/* start config */
#define __DEBUG_MODE            OFF

/* section wifi */
#define HP_STR_SSID             "NSA.Host.2.4"
#define HP_STR_PWD              "Alexandru1"

/* section telnet */
#define TELNET_SERVER_NAME      "3DP_HostPrint"

#define TELNET_TCP_PORT         7050
#define TELNET_DNS_PORT         53

/* section pins */
#define PIN_CAM_FLASH           4
#define PIN_LED                 33

/* SPI SD Card */
#define PIN_SD_CLK              14
#define PIN_SD_SS               13
#define PIN_SD_MISO             2
#define PIN_SD_MOSI             15

/* section misc */
#if __DEBUG_MODE == ON
    #define DBG_OUTPUT_PORT     Serial
    #define LOG_Println(x)      DBG_OUTPUT_PORT.println((x))
    #define LOG_Init()          DBG_OUTPUT_PORT.begin(115200);\
                                DBG_OUTPUT_PORT.setDebugOutput(true)
#else
    #define LOG_Println(x)
    #define LOG_Init()
#endif

/* section buffers */

#define HP_MAX_SAVED_CMD        200u
#define HP_CMD_QUEUE_SIZE       100u
#define HP_CMD_SLOTS            3u
#define HP_PRELOAD_CMD_NUM      10

#endif