#ifndef TELNET_SERVICE_h
#define TELNET_SERVICE_h

#include <WiFi.h>
#include "Config.h"
#include "Util.h"
#include "MessageQueue.h"

#define TELNET_SERVER_NAME  "3DP_HostPrint"
#define TX_BUFF_SIZE        1000

class TelnetService : public Print
{
private:
    // DNSServer _dnsServer;
    WiFiServer* _server;
    WiFiClient _client;
    MessageQueue<String> _messageList;
    bool _hasClient;
    uint16_t _port;
    char _txBuffer[TX_BUFF_SIZE];
    size_t _txBuffIdx;

public:
    TelnetService()
    {
        #if (ON == ENABLE_TELNET)
        _server = new WiFiServer();
        _client = WiFiClient();
        _port = 23;
        _hasClient = false;
        _txBuffIdx = 0;
        #endif
    }
    ~TelnetService()
    {
    }
    void begin(uint16_t port)
    {
        #if (ON == ENABLE_TELNET)
        if(port)
        {
            _port = port;
        }
        _server->begin(_port);
        _server->setNoDelay(true);
        #endif
    };
    void loop();
    size_t write(uint8_t* buff, size_t len);
    
    inline size_t write(uint8_t c)
    {
        #if (ON == ENABLE_TELNET)
            return write(&c, 1);
        #else
            return 1;
        #endif
    }
    inline size_t write(const char * s)
    {
        return write((uint8_t*) s, strlen(s));
    }
    inline size_t write(unsigned long n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(long n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(unsigned int n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(int n)
    {
        return write((uint8_t) n);
    }
};

extern TelnetService telnetService;
#endif /* TELNET_SERVER_h */