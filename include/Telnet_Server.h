#ifndef TELNET_SERVER_h
#define TELNET_SERVER_h

#include "WiFi.h"
#include "Config.h"
#include <Message_Queue.h>

#define TELNET_SERVER_NAME "3DP_HostPrint"
#define TELNET_LOG_SIZE 50

#define TELNET_PORT 23

class TelnetLogger : public Print
{
private:
    // DNSServer _dnsServer;
    WiFiServer _server;
    WiFiClient _client;
    MessageQueue<String> _messageList;

public:
    void begin()
    {
        _server.begin(TELNET_PORT);
        _server.setNoDelay(true);
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