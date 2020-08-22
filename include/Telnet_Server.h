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
    /* transmit to clients every 300ms */
    const uint16_t TRANSMIT_INTERVAL = 500;
public:
    TelnetLogger()
    {
        _server = new WiFiServer();
        _client = WiFiClient();
        _nextTransmit = 0;
        _port = 23;
    }
    void begin(uint16_t port)
    {
        if(port)
        {
            _port = port;
        }
        _server->begin(_port);
        _server->setNoDelay(true);
    };
    void loop();
    size_t write(uint8_t c)
    {
        if (_messageList.size() < TELNET_LOG_SIZE)
        {
            _messageList.push((String)c);
        }
        return 1;
    }
    size_t write(String str)
    {
        if (_messageList.size() < TELNET_LOG_SIZE)
        {
            _messageList.push(str);
            return str.length();
        }
        return 0;
    }
    void write(int no)
    {
        if (_messageList.size() < TELNET_LOG_SIZE)
        {
            _messageList.push((String)no);
        }
    }
    void write(uint32_t no)
    {
        if (_messageList.size() < TELNET_LOG_SIZE)
        {
            _messageList.push((String)no);
        }
    }

    void write(boolean b)
    {
        if (b)
        {
            write((String) "true");
        }
        else
        {
            write((String) "false");
        }
    }

    void println(String str)
    {
        write(str + '\n');
    }

    void println(uint32_t no)
    {
        write((String)no + '\n');
    }
    void println(boolean b)
    {
        write(b);
    }
};

extern TelnetLogger TelnetLog;
#endif /* TELNET_SERVER_h */