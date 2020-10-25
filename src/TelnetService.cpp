#include "TelnetService.h"

TelnetService telnetService;

size_t TelnetService::write(uint8_t *buff, size_t len)
{
    if(len > TX_BUFF_SIZE)
    {
        len = TX_BUFF_SIZE-2;
    }
    for(size_t idx = 0; idx < len; idx++)
    {
        _txBuffer[_txBuffIdx] = (char)buff[idx];
        if((buff[idx] == '\n') || 
            (_txBuffIdx == (TX_BUFF_SIZE-2)))
        {
            _txBuffer[_txBuffIdx+1] = '\0';
            _messageList.push(((String)_txBuffer));

            _txBuffIdx = 0;
        }
        else
        {
            _txBuffIdx++;
        }
    }
    return len;
}

void TelnetService::loop()
{
#if (ON == ENABLE_DEBUG)
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

    if (_client.connected())
    {
        if(false == _messageList.isempty())
        {
            _client.print(_messageList.front());
            _messageList.pop();
        }
    }
#endif
};