#include "Telnet_Server.h"

TelnetLogger TelnetLog;

void TelnetLogger::loop()
{
    if(_nextTransmit < millis())
    {
        if (_server->hasClient())
        {
            if (_client.connected())
            {   
                /* reject this new connection */
                _server->available().stop();
            }
            else
            {
                _client = _server->available();
            }
        }

        if (!_messageList.isempty() && _client.connected())
        {
            _client.print(_messageList.front());
            _messageList.pop();
        }
        // Serial.printf("M all heap:%d\r\nM all PSRSM:%d\r\nm free heap:%d\r\nm free PSRAM:%d\r\nfree HEAP:%d\r\nfree PSRAM:%d\r\n", 
        //     ESP.getMaxAllocHeap(), 
        //     ESP.getMaxAllocPsram(), 
        //     ESP.getMinFreeHeap(), 
        //     ESP.getMinFreePsram(),
        //     ESP.getFreeHeap(),
        //     ESP.getFreePsram());

        _nextTransmit = millis() + TRANSMIT_INTERVAL;
    }
    // _dnsServer.processNextRequest(); 
};