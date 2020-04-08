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

    while ((_commands.freeSlots() > maxSlots / 2) && (_commands.freeSlots() > HP_CMD_SLOTS))
    {
        if (_file.available() > 0)
        {
            line = _file.readStringUntil(LF_CHAR);
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
                    return line;                    
                }
            }
            break;

            default:
            break;
        }
    }
    return "";
}

inline void PrintHandler::addCommand(String& command)
{
    String pLine = parseLine(command);
    
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
    String textLine;
    // bool found_O = false;  
        while(_serial->available())
        {
            char c = (char)_serial->read();
            rcv += c;

            if(rcv.indexOf("ok") != -1)
            {
                _ackRcv = true;
            }

        // int endIdx = rcv.indexOf('\n');
        // int startIdx = 0;
        // while(endIdx != -1)
        // {
        //     rcv.replace("\r", "");
        //     endIdx = rcv.indexOf('\n');
        //     textLine = rcv.substring(startIdx, endIdx);
        //     rcv = rcv.substring(endIdx+1, rcv.length());
        //     startIdx = endIdx;

        //     if(textLine.indexOf("ok") != -1)
        //     {
        //         _ackRcv = true;
        //         break;
        //     }
        // }


        // char data = (char)_serial->read();
        // if(data != LF_CHAR)
        // {
        //     rcv += data;
        // }
        // /* try to catch faster an "OK" from printer */
        // if((data == 'o') || (data == 'O'))
        // {
        //     found_O = true;
        // }
        // if(((data == 'k') || (data == 'K')) && (found_O == true))
        // {
        //     _ackRcv = true;
        //     found_O = false;
        // }
        if(_aWs->availableForWriteAll())
        {
            _aWs->textAll(rcv);
        }
    }
    // if(rcv.length() > 0)
    // {
        // else
        // {
        //     _printerMsg.push(rcv);
        // }
    // }
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
            // _serial->flush();
            _aWs->textAll(printerCommand);
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
            line = _file.readStringUntil(LF_CHAR);
            line.trim();
            addCommand(line);
            
            _estCompPrc = 100U - (bytesAvail * 100U / _fileSize);
        }
    }    
    else
    {
        _file.close();
        _printStarted = false;
        _printCompleted = true;
    }
    
}

void PrintHandler::loopTx()
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
            if (!_printCompleted)
            {
                parseFile();

                if (millis() >= _prgTout)
                {
                    writeProgress(_estCompPrc);
                    _prgTout = millis() + TOUT_PROGRESS;
                }
            }
            else
            {
                _state = PH_STATE_IDLE;
            }
        break;

        default:
        break;
    }
    processSerialTx();
}

void PrintHandler::loopRx()
{
    processSerialRx();
}