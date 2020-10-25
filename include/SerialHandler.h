#ifndef SERIAL_HANDLER_h
#define SERIAL_HANDLER_h

#include "Config.h"

class SerialHandler
{
    private:
        // uint8_t _rxBuffer[HP_SERIAL_RX_BUFFER_SIZE];
        // size_t _rxBufferSize;    
        void rxProcessBuffer(uint8_t* buff, size_t len);

    public:
        void begin();
        void rxProcess();
        size_t write(uint8_t* buff, size_t len);
        void write(uint8_t no);        
        int available()
        {
            return PRINTER_SERIAL.available();
        }
        void updateBaudRate(uint32_t baud)
        {
            PRINTER_SERIAL.updateBaudRate(baud);
        }
        int availableForWrite()
        {
            return PRINTER_SERIAL.availableForWrite();
        }
        size_t readBytes(uint8_t* buff, size_t len)
        {
            return PRINTER_SERIAL.readBytes(buff, len);
        }


};

extern SerialHandler serialHandler;

#endif