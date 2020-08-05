#include "Telnet_Server.h"

TelnetLogger TelnetLog;
std::vector<AsyncClient*> TelnetLogger::_telnetClients;

 /* clients events */
void TelnetLogger::tcpServerErrorHandler(void* arg, AsyncClient* client, int8_t error) {
	Serial.printf("\n connection error %s from client %s \n", client->errorToString(error), client->remoteIP().toString().c_str());
}

void TelnetLogger::tcpServerDataHandler(void* arg, AsyncClient* client, void *data, size_t len) {
	Serial.printf("\n data received from client %s \n", client->remoteIP().toString().c_str());
	Serial.write((uint8_t*)data, len);

	// reply to client
	if (client->space() > 400 && client->canSend()) {
		char reply[400];
		sprintf(reply, "this is a huuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuooooooooooooooouuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuge reply from %s", TELNET_SERVER_NAME);
		client->add(reply, strlen(reply));
		client->send();
	}
}

void TelnetLogger::tcpServerDisconnectHandler(void* arg, AsyncClient* client) 
{
	Serial.printf("\n client %s disconnected \n", client->remoteIP().toString().c_str());
}

void TelnetLogger::tcpServerTimeoutHandler(void* arg, AsyncClient* client, uint32_t time) 
{
	Serial.printf("\n client ACK timeout ip: %s \n", client->remoteIP().toString().c_str());
}


void TelnetLogger::tcpServerConnectionHandler(void* arg, AsyncClient* client)
{
	TelnetLogger::_telnetClients.push_back(client);

	client->onData(tcpServerDataHandler, NULL);
	client->onError(tcpServerErrorHandler, NULL);
	client->onDisconnect(tcpServerDisconnectHandler, NULL);
	client->onTimeout(tcpServerTimeoutHandler, NULL);
}


