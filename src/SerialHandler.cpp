#include "esp_task_wdt.h"

#include "SerialHandler.h"
#include "GcodeHost.h"
#include "Util.h"
#include "Log.h"

static const TickType_t SERIAL_TX_DELAY = 2 / portTICK_PERIOD_MS;

static const uint16_t BAUD_RATES_COUNT = 1;
static const uint32_t BAUD_RATES[BAUD_RATES_COUNT] = {115200};

SerialHandler serialHandler;

void SerialHandler::rxProcess()
{
    size_t rxLen = available();
    /* have pending data */
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
/*
*@brief Write a buffer of bytes over serial
*@param buff pointer to the data buffer
*@param len size in bytes of the dat to be written
*@return the number of bytes written
*/
size_t SerialHandler::write(uint8_t* buff, size_t len)
{
    /* current data fits the TX fifo */
    if (availableForWrite() >= len) 
    {
        return PRINTER_SERIAL.write(buff, len);
    } 
    else 
    {
        size_t bytesToSend = len;
        size_t bytesSent = 0;
        uint8_t* tmpBuff = (uint8_t*)buff;
        /* loop until all is sent */
        while (bytesToSend > 0) 
        {
            size_t availWrite = availableForWrite();
            if(availWrite > 0) 
            {
                size_t len = availWrite;
                if(availWrite >= bytesToSend)
                {
                    len = bytesToSend;
                }
                /* in case less is sent */
                availWrite = PRINTER_SERIAL.write(&tmpBuff[bytesSent], len);
                bytesToSend -= availWrite;
                bytesSent += availWrite;
            }
            else 
            {
                /* set task to BLOCK so the serial TX fifo gets freed */
                vTaskDelay(SERIAL_TX_DELAY);
            }
        }
        return bytesSent;
    }
}

/*
*@brief Initialize the SerialHandler object call it in init sequence
*@param N/A
*@return N/A
*/
void SerialHandler::begin()
{
    PRINTER_SERIAL.setRxBufferSize(HP_SERIAL_RX_QUEUE_SIZE);
    
#if ((ON == ENABLE_DEBUG) && (ON == ENABLE_TELNET)) || (OFF == ENABLE_DEBUG)
    PRINTER_SERIAL.begin(BAUD_RATES[0], SERIAL_8N1);
#endif
}