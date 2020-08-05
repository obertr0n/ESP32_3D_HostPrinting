#ifndef TELNET_SERVER_h
#define TELNET_SERVER_h

#include "WiFi.h"
#include "DNSServer.h"
#include "AsyncTCP.h"
#include "HP_Config.h"

class TelnetLogger : public Print
{
    private:
        DNSServer _dnsServer;
        static std::vector<AsyncClient*> _telnetClients;

        static void tcpServerErrorHandler(void* arg, AsyncClient* client, int8_t error);
        static void tcpServerDataHandler(void* arg, AsyncClient* client, void *data, size_t len);
        static void tcpServerDisconnectHandler(void* arg, AsyncClient* client);
        static void tcpServerTimeoutHandler(void* arg, AsyncClient* client, uint32_t time);
        static void tcpServerConnectionHandler(void* arg, AsyncClient* client);
        
    public:
        void begin()
        {
            if (!_dnsServer.start(TELNET_DNS_PORT, TELNET_SERVER_NAME, WiFi.localIP()))
	            LOG_Println("\n failed to start dns service \n");
            
            AsyncServer* tcpServer = new AsyncServer(TELNET_TCP_PORT);
            tcpServer->onClient(tcpServerConnectionHandler, tcpServer);
            tcpServer->begin();
        };
        void loop()
        {            
            _dnsServer.processNextRequest();
        };
        size_t write(uint8_t c)
        {
            return 0;
        }
};

extern TelnetLogger TelnetLog;
#endif /* TELNET_SERVER_h */