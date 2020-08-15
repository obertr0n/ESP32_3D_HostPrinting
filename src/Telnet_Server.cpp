#include "Telnet_Server.h"

TelnetLogger TelnetLog;

void TelnetLogger::loop()
{
    if (_server.hasClient())
    {
        if (_client.connected())
        {   
            /* reject this new connection */
            _server.available().stop();
        }
        else
        {
            _client = _server.available();
        }
    }

    if (!_messageList.empty())
    {
        //_client.print("_messageList.front()");
        // _messageList.pop();
    }
    if (_client.connected())
    {
        _client.print("_messageList.front()");
        delay(500);
    }
    // _dnsServer.processNextRequest(); 
};

// std::vector<AsyncClient*> TelnetLogger::_telnetClients;
//  /* clients events */
// void TelnetLogger::tcpServerErrorHandler(void* arg, AsyncClient* client, int8_t error) 
// {
//     // Serial.printf("\n client with ip: %s had error %d \n", client->remoteIP().toString().c_str(), error);
// }

// void TelnetLogger::tcpServerDataHandler(void* arg, AsyncClient* client, void *data, size_t len) 
// {	
//     // Serial.printf("\n client with ip: %s sent something \n", client->remoteIP().toString().c_str());
//     if (client->space() > 30 && client->canSend()) 
//     {
//         char reply[40];
//         sprintf(reply, "Connected to %s", TELNET_SERVER_NAME);
//         client->add(reply, strlen(reply));
//         client->send();
//     }
// }

// void TelnetLogger::tcpServerDisconnectHandler(void* arg, AsyncClient* client) 
// {
//     // Serial.printf("\n client with ip: %s disconnected\n", client->remoteIP().toString().c_str());
//     std::vector<AsyncClient*>::iterator it;
//     for (it = _telnetClients.begin() ; it != _telnetClients.end(); ++it)
//     {
//         if((*it)->remoteIP() == client->remoteIP())
//         {
//             _telnetClients.erase(it);
//         }
//     }
// }

// void TelnetLogger::tcpServerTimeoutHandler(void* arg, AsyncClient* client, uint32_t time) 
// {
//     // Serial.printf("\n client timmedout with ip: %s \n", client->remoteIP().toString().c_str());
// }

// void TelnetLogger::tcpServerConnectionHandler(void* arg, AsyncClient* client)
// {
//     // Serial.printf("\n client connected with ip: %s \n", client->remoteIP().toString().c_str());
//     TelnetLogger::_telnetClients.push_back(client);

//     client->onData(tcpServerDataHandler, NULL);
//     client->onError(tcpServerErrorHandler, NULL);
//     client->onDisconnect(tcpServerDisconnectHandler, NULL);
//     client->onTimeout(tcpServerTimeoutHandler, NULL);
// }


