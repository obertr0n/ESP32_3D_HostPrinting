#include "Telnet_Server.h"

TelnetLogger TelnetLog;

void TelnetLogger::loop()
{
    
    #if (ON == __DEBUG_MODE)
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
        _nextTransmit = millis() + TRANSMIT_INTERVAL;
    }
    #endif
};