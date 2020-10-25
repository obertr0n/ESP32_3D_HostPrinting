#include "esp_task_wdt.h"

#include "SerialHandler.h"
#include "GcodeHost.h"
#include "Util.h"
#include "Log.h"

#define SERIAL_TX_TIMEOUT       100

static const uint16_t BAUD_RATES_COUNT = 1;
static const uint32_t BAUD_RATES[BAUD_RATES_COUNT] = {115200};

SerialHandler serialHandler;


void SerialHandler::rxProcess()
{
    size_t rxLen = available();
    // pending data
    if(rxLen > 0)
    {
        uint8_t* dBuff = (uint8_t*) malloc(sizeof(uint8_t) * (rxLen + 1));
        if(dBuff != NULL)
        {
            size_t bytesRead = readBytes(dBuff, rxLen);
            if(bytesRead > 0)
            {
                dBuff[rxLen] = '\0';
                /* the function will also free the buffer to ensure enough heap */
                // gcodeHost.processSerialReply(dBuff, rxLen+1);
            }
            free(dBuff);
        }
    }
}

size_t SerialHandler::write(uint8_t* buff, size_t len)
{
    if (availableForWrite() >= len) 
    {
        return PRINTER_SERIAL.write(buff, len);
    } 
    else 
    {
        size_t bytesToSend = len;
        size_t bytesSent = 0;
        uint8_t* tmpBuff = (uint8_t*)buff;
        uint32_t start = millis();
        //loop until all is sent or timeout
        while (bytesToSend > 0 && ((millis() - start) < SERIAL_TX_TIMEOUT)) 
        {
            size_t availableBytes = availableForWrite();
            if(availableBytes > 0) 
            {
                size_t len = availableBytes;
                if(availableBytes >= bytesToSend)
                {
                    len = bytesToSend;
                }
                //in case less is sent
                availableBytes = PRINTER_SERIAL.write(&tmpBuff[bytesSent], len);
                bytesToSend -= availableBytes;
                bytesSent += availableBytes;
                start = millis();
            }
            else 
            {
                Util.safeDelay(5);
            }
        }
        return bytesSent;
    }
}

void SerialHandler::begin()
{
    PRINTER_SERIAL.setRxBufferSize(HP_SERIAL_RX_QUEUE_SIZE);
    
    #if ((ON == ENABLE_DEBUG) && (ON == ENABLE_TELNET)) || (OFF == ENABLE_DEBUG)
    PRINTER_SERIAL.begin(BAUD_RATES[0], SERIAL_8N1);
    #endif
}