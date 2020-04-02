#include "Print_Handler.h"

void PrintHandler::begin(File& file, HardwareSerial& port, uint32_t baud)
{
    _file = file;
    _fileSize = _file.size();
    _serial = port;

    _serial.begin(baud);
}

void PrintHandler::parseFile()
{
    String line;
    size_t bytesAvail = 0;
    if(PS_PRINTING == _state)
    {
        bytesAvail = _file.available();
        if((bytesAvail > 0) && (_commands.freeSlots() > CMD_SLOTS))
        {
            line = _file.readStringUntil(EOL_CHAR);
            // int idx = line.indexOf(COMMENT_CHAR);
            // if(idx > 0)
            // {
            //     line = line.substring(0, idx);
            // }
            // else if(line.length() > 2)
            // {
            //     _commands.push(line);
            // }
            // /* line must start with a G or an M command*/
            // if(line[0] == 'M' || line[0] == 'G')
            // {
                
            // }
            
            uint8_t prc = _fileSize * 100 / (_fileSize - bytesAvail);
            writeProgress(prc);

            _commands.push(line);
        }
    }
}

void PrintHandler::sendToPrinter()
{
    // _serial.write()
}