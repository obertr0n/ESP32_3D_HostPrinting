#ifndef TELNET_SERVER_h
#define TELNET_SERVER_h

#include "WiFi.h"
#include "Config.h"
#include <Message_Queue.h>

#define TELNET_SERVER_NAME  "3DP_HostPrint"
#define TELNET_LOG_SIZE     HP_CMD_QUEUE_SIZE

class TelnetLogger : public Print
{
private:
    // DNSServer _dnsServer;
    WiFiServer* _server;
    WiFiClient _client;
    MessageQueue<String> _messageList;
    uint32_t _nextTransmit;
    uint16_t _port;
    /* transmit to clients every 200ms */
    const uint16_t TRANSMIT_INTERVAL = 200;
public:
    TelnetLogger()
    {
        #if (ON == __DEBUG_MODE)
        _server = new WiFiServer();
        _client = WiFiClient();
        _nextTransmit = 0;
        _port = 23;
        #endif
    }
    void begin(uint16_t port)
    {
        #if (ON == __DEBUG_MODE)
        if(port)
        {
            _port = port;
        }
        _server->begin(_port);
        _server->setNoDelay(true);
        #endif
    };
    void loop();
    size_t write(uint8_t c)
    {
        #if (ON == __DEBUG_MODE)
        if (_messageList.size() < TELNET_LOG_SIZE)
        {
            _messageList.push((String)c);
        }
        #endif
        return 1;
    }
    size_t write(String str)
    {
        #if (ON == __DEBUG_MODE)
        if (_messageList.size() < TELNET_LOG_SIZE)
        {
            _messageList.push(str);
            return str.length();
        }
        #endif
        return 0;
    }
    void write(int no)
    {        
        #if (ON == __DEBUG_MODE)
        if (_messageList.size() < TELNET_LOG_SIZE)
        {
            _messageList.push((String)no);
        }
        #endif
    }
    void write(uint32_t no)
    {
        #if (ON == __DEBUG_MODE)
        if (_messageList.size() < TELNET_LOG_SIZE)
        {
            _messageList.push((String)no);
        }
        #endif
    }
    void write(boolean b)
    {
        #if (ON == __DEBUG_MODE)
        if (b)
        {
            write((String) "true\n");
        }
        else
        {
            write((String) "false\n");
        }
        #endif
    }

    void println(String str)
    {
        write(str + "\n");
    }

    void println(uint32_t no)
    {
        write((String)no + "\n");
    }
};

extern TelnetLogger TelnetLog;
#endif /* TELNET_SERVER_h */