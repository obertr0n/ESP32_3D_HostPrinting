#ifndef CONFIG_h
#define CONFIG_h

#include <HardwareSerial.h>

#define ON  1U
#define OFF 0U

/* section soft AP */
#define AP_DEFAULT_SSID             "ESPHostPrinting"
#define AP_DEFAULT_PASS             "welcomeESP"

/* section serial */
#define PRINTER_SERIAL              Serial

/* section storage */
#define ENABLE_SD_CARD              ON

/* start config */
#define ENABLE_DEBUG                ON
#define ENABLE_TELNET               ON

/* section pins */
#define PIN_CAM_FLASH               4
#define PIN_LED                     33

/* SPI SD Card */
#define PIN_SD_CLK                  14
#define PIN_SD_SS                   13
#define PIN_SD_MISO                 2
#define PIN_SD_MOSI                 15

/* section buffers */
#define HP_LOG_QUEUE_SIZE           400u

#define HP_MAX_SAVED_CMD            550u    // resend command queue
#define HP_CMD_QUEUE_SIZE           500u    // gcode command queue

#define HP_SERIAL_RX_QUEUE_SIZE     768u // Rx queue size
#define HP_SERIAL_RX_BUFFER_SIZE    (HP_SERIAL_RX_QUEUE_SIZE * 2) // Rx queue size

/* section inline debugger (FTDI FT232H) */
/* setting this to ON will disable the SD Card because of conflicting pins */
#define ENABLE_ESP_DEBUGGER         ON

#if (ON == ENABLE_ESP_DEBUGGER)
    #undef ENABLE_SD_CARD
    #define ENABLE_SD_CARD OFF
#endif

#endif /* CONFIG_h */