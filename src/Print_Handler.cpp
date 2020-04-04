#include "Print_Handler.h"
#include "HP_Config.h"

void PrintHandler::begin(uint32_t baud, AsyncWebSocket* ws)
{
    _serial->begin(baud);
    _aWs = ws;
}

void PrintHandler::preBuffer()
{
    String line;
    uint32_t maxSlots = _commands.maxSize();

    while ((_commands.freeSlots() > HP_CMD_SLOTS) && (_commands.freeSlots() > maxSlots / 2))
    {
        if (_file.available() > 0)
        {
            line = _file.readStringUntil(EOL_CHAR);
            addCommand(line);
        }
    }
}

String PrintHandler::parseLine(String& line)
{
    /* try and remove the whitespaces */
    line.trim();
    /* supress the lines that start with a comment */
    if(line[0] != COMMENT_CHAR)
    {
        /* line starts with one of the accepted gcode commands */
        switch (line[0])
        {
            case M_COMMAND: 
            case G_COMMAND:
            case T_COMMAND:
            {
                int idx = line.indexOf(COMMENT_CHAR);
                if(idx < 0)
                {
                    return line;
                }
                /* we must have more than 2 chars before a comment char */
                else if(idx > 2)
                {
                    line = line.substring(0, idx);
                    if(line.length() > 2)
                    {
                        return line;
                    }
                    
                }
                break;
            }
            default:
            break;
        }
    }
    return "";
}

inline void PrintHandler::addCommand(String& command)
{
    String pLine;
    pLine = parseLine(command);
    
    if (pLine != "")
    {
        _commands.push(pLine);
    }
}

bool PrintHandler::send(String& command)
{
    if(!isPrinting())
    {
        _serial->println(command);
        return true;
    }
    
    return false;
}

void PrintHandler::processSerialRx()
{
    String rcv;
    bool found_O = false;
    while(_serial->available() > 0)
    {
        int data = _serial->read();
        /* try to catch faster an "OK" from printer */
        if((data == 'o') || (data == 'O'))
        {
            found_O = true;
        }
        if(((data == 'k') || (data == 'K')) && (found_O == true))
        {
            _ackRcv = true;
        }
        rcv += (char)data;
    }
    if(rcv.length() > 0)
    {
        if(_aWs->availableForWriteAll())
        {
            _aWs->textAll(rcv);
        }
        else
        {
            _printerMsg.push(rcv);
        }
    }
}

void PrintHandler::decodePrinterMsg()
{

}

void PrintHandler::processSerialTx()
{
    String printerCommand;

    if(_ackRcv)
    {
        if(_commands.pop(printerCommand))
        {
            _serial->println(printerCommand);
            /* reset it only when the transmission is done? */
            _ackRcv = false;
        }
    }
}

void PrintHandler::parseFile()
{
    String line;
    size_t bytesAvail = 0;

    bytesAvail = _file.available();
    if (bytesAvail > 0)
    {
        if (_commands.freeSlots() > HP_CMD_SLOTS)
        {
            line = _file.readStringUntil(EOL_CHAR);
            addCommand(line);
            
            _estCompPrc = 100 - (bytesAvail * 100 / _fileSize);

            // _serial->printf("Total: %d avail: %d percent %d", _fileSize, bytesAvail, _estCompPrc);
        }
    }
    else
    {
        _printStarted = false;
        _printCompleted = true;
    }
    
}

void PrintHandler::loop()
{
    switch(_state)
    {
        case PH_STATE_PRINT_REQ:
            _printStarted = true;
            _printCompleted = false;
            preBuffer();
            _ackRcv = true;
            _state = PH_STATE_PRINTING;
        break;

        case PH_STATE_PRINTING:
            parseFile();
            
            if(millis() >= _prgTout)
            {
                writeProgress(_estCompPrc);
                _prgTout = millis() + TOUT_PROGRESS;
            }
        break;

        default:
        break;
    }

    processSerialRx();
    processSerialTx();
}