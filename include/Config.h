#ifndef CONFIG_h
#define CONFIG_h

#define ON  1U
#define OFF 0U

/* section soft AP */
#define AP_DEFAULT_SSID         "ESPHostPrinting"
#define AP_DEFAULT_PASS         "welcomeESP"

/* section serial */
#define PRINTER_SERIAL          Serial

/* start config */
#define __DEBUG_MODE            OFF
#define USE_TELNET              ON

/* section pins */
#define PIN_CAM_FLASH           4
#define PIN_LED                 33

/* SPI SD Card */
#define PIN_SD_CLK              14
#define PIN_SD_SS               13
#define PIN_SD_MISO             2
#define PIN_SD_MOSI             15

/* section buffers */

#define HP_MAX_SAVED_CMD        550u
#define HP_CMD_QUEUE_SIZE       500u
#define HP_CMD_SLOTS            3u
#define HP_PRELOAD_CMD_NUM      50


#endif /* CONFIG_h */