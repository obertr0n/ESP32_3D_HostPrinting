#ifndef HP_UTIL_h
#define HP_UTIL_h

#include "Arduino.h"

extern void util_init();
extern void util_telnetLoop();
extern String util_millisToTime();
extern void util_telnetSend(String line);
extern void util_blink_status();
extern void listTasks();
extern String util_getIP();

#endif