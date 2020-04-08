#include "Print_Handler.h"
#include "HP_Config.h"

void PrintHandler::begin(uint32_t baud, AsyncWebSocket *ws)
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

String PrintHandler::parseLine(String &line)
{
    /* supress the lines that start with a comment */
    if (line[0] != COMMENT_CHAR)
    {
        /* line starts with one of the accepted gcode commands */
        switch (line[0])
        {
        case M_COMMAND:
        case G_COMMAND:
        case T_COMMAND:
        {
            int idx = line.indexOf(COMMENT_CHAR);
            if (idx < 0)
            {
                return line;
            }
            /* we must have more than 2 chars before a comment char */
            else if (idx > 2)
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

inline void PrintHandler::addCommand(String &command)
{
    String pLine = parseLine(command);

    if (pLine != "")
    {
        _commands.push(pLine);
    }
}

void PrintHandler::detectPrinter()
{
}

bool PrintHandler::send(String &command)
{
    if (!isPrinting())
    {
        write(command);
        return true;
    }

    return false;
}

/*  T:110.03 /190.00 B:50.68 /0.00 @:0 B@:0 W:? */
bool parseTemp(const String &line)
{
    int t_pos = line.indexOf(" T:");
    if(t_pos >= 0)
    {
        int tslh_pos = line.indexOf(" /", t_pos);
    }
    int b_pos = line.indexOf(" B:");
    if(b_pos >= 0)
    {
        int bslh_pos = line.indexOf(" /", b_pos);
    }

    return false;
}

bool parseFwinfo(const String &line)
{
    return false;
}

bool parse503(const String &line)
{
    return false;
}

void PrintHandler::processSerialRx()
{
    String stringBuff;
    String textLine;
    // bool found_O = false;
    if (_serial->available() > 0)
    {
        int len = _serial->available();
        uint8_t cbuff[len + 1];

        _serial->readBytes(cbuff, len);
        /* to string */
        cbuff[len + 1] = '\0';

        stringBuff = (char *)cbuff;
        while (stringBuff.indexOf("\n") > 0)
        {
            stringBuff.replace("\r", "");
            /* get a line */
            int endIdx = stringBuff.indexOf("\n");
            String line = stringBuff.substring(0, endIdx);
            // line.trim();

            if (line == "ok")
            {
                _ackRcv = true;
                LOG_Println("Found OK");
            }
            else
            {
                if (parseTemp(line))
                {
                }
                if (!_printStarted)
                {
                    if (parseFwinfo(line))
                    {
                    }
                    else if (parse503(line))
                    {
                    }
                }
                else
                {
                    /* code */
                }
            }

            if (_aWs->availableForWriteAll())
            {
                _aWs->textAll(line);
            }
            stringBuff = stringBuff.substring(endIdx + 1, stringBuff.length());
        }
    }
}

void PrintHandler::decodePrinterMsg()
{
}

void PrintHandler::processSerialTx()
{
    String printerCommand;

    if (_ackRcv)
    {
        printerCommand = _commands.pop();
        if (printerCommand != "")
        {
            write(printerCommand);
            /* reset it only when the transmission is done? */
            _ackRcv = false;
            if (_aWs->availableForWriteAll())
            {
                _aWs->textAll(printerCommand);
            }
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
            // line.trim();
            // line.replace("\r", "");
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
    if (_printerConnected)
    {
        switch (_state)
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
}

void PrintHandler::loopRx()
{
    if (_printerConnected)
    {
        processSerialRx();
    }
}