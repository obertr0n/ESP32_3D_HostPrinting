#include "Print_Handler.h"
#include "HP_Config.h"

void PrintHandler::begin(AsyncWebSocket *ws)
{
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
    static uint16_t baudIndex = 0;

    if((!_printerConnected) && (millis() >= _commTout))
    {
        _serial->begin(BAUD_RATES[baudIndex]);
        sendM115();
        
        // _serial->println(baudIndex);
        // _serial->println(BAUD_RATES[baudIndex]);

        baudIndex++;
        baudIndex %= BAUD_RATES_COUNT;
        resetCommTimeout();
    }
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
bool PrintHandler::parseTemp(const String &line)
{
    bool foundTemp = false;
    _serial->println(line);
    
    int t_pos = line.indexOf("T:");
    if (t_pos >= 0)
    {
        int tslh_pos = line.indexOf("/", t_pos);
        if(tslh_pos >= 0)
        {
            String h_temp = line.substring(t_pos+2, tslh_pos);
            _serial->println(h_temp);
            foundTemp = true;
        }
        int b_pos = line.indexOf("B:");
        if (b_pos >= 0)
        {        
            String h_preset = line.substring(tslh_pos+1, b_pos);
            _serial->println(h_preset);
            foundTemp = true;

            int bslh_pos = line.indexOf(" /", b_pos);
            if(bslh_pos >= 0)
            {
                String b_temp = line.substring(b_pos+2, bslh_pos);
                _serial->println(b_temp);
                foundTemp = true;

                int at_pos = line.indexOf(" @");
                if(at_pos >= 0)
                {
                    String b_preset = line.substring(bslh_pos+2, at_pos);
                    _serial->println(b_preset);
                    foundTemp = true;
                }
            }
        }
    }

    return foundTemp;
}

bool PrintHandler::parse503(const String &line)
{
    return false;
}

/*
    FIRMWARE_NAME:Marlin bugfix-2.0.x (GitHub) SOURCE_CODE_URL:https://github.com/MarlinFirmware/Marlin 
    PROTOCOL_VERSION:1.0 
    MACHINE_TYPE:3D Printer 
    EXTRUDER_COUNT:1 
    UUID:cede2a2f-41a2-4748-9b12-c55c62f367ff
    Cap:SERIAL_XON_XOFF:0
    Cap:BINARY_FILE_TRANSFER:0
    Cap:EEPROM:1
    Cap:VOLUMETRIC:1
    Cap:AUTOREPORT_TEMP:1
    Cap:PROGRESS:0
    Cap:PRINT_JOB:1
    Cap:AUTOLEVEL:1
    Cap:Z_PROBE:1
    Cap:LEVELING_DATA:1
    Cap:BUILD_PERCENT:0
    Cap:SOFTWARE_POWER:0
    Cap:TOGGLE_LIGHTS:0
    Cap:CASE_LIGHT_BRIGHTNESS:0
    Cap:EMERGENCY_PARSER:0
    Cap:PROMPT_SUPPORT:0
    Cap:AUTOREPORT_SD_STATUS:0
    Cap:THERMAL_PROTECTION:1
    Cap:MOTION_MODES:0
    Cap:CHAMBER_TEMPERATURE:0 */
bool PrintHandler::parseM115(const String &line)
{
    bool retVal = false;

    int fwname_pos = line.indexOf("FIRMWARE_NAME:");
    if(fwname_pos >= 0)
    {
        String fwName = line.substring(fwname_pos, fwname_pos+7);
        _serial->println(fwName);

        retVal = true;
    }
    int extruderCnt_pos = line.indexOf("EXTRUDER_COUNT:");
    if(extruderCnt_pos >= 0)
    {
        String extCnt = line.substring(extruderCnt_pos, 1);
        _serial->println(extCnt);

        retVal = true;
    }

    return retVal;
}

void PrintHandler::processSerialRx()
{
    static int lineStartPos = 0;
    static String serialResponse;

    while (_serial->available())
    {
        char ch = (char)_serial->read();
        if (ch != '\n')
        {
            serialResponse += ch;
        }
        else
        {
            bool incompleteResponse = false;
            if(PH_M115 == _prevCmd)
            {
                if(parseM115(serialResponse))
                {
                    _state = PH_STATE_CONNECTED;
                    _printerConnected = true;
                }
            }
            if (serialResponse.startsWith("ok", lineStartPos))
            {
                _ackRcv = true;
                incompleteResponse = false;
            }
            else if (_printerConnected)
            {
                if(parseTemp(serialResponse))
                {
                    incompleteResponse = false;
                }
                if (serialResponse.startsWith("echo:busy"))
                {
                    _ackRcv = false;
                }
                // else if (serialResponse.startsWith("echo: cold extrusion prevented"))
                // {
                //     // To do: Pause sending gcode, or do something similar
                //     // responseDetail = "cold extrusion";
                // }
                else if (serialResponse.startsWith("Error:"))
                {
                    _ackRcv = false;
                    _printCanceled = true;
                }
                else
                {
                    incompleteResponse = true;
                }
            }
            else
            {
                incompleteResponse = true;
            }

            // transmit back what we received
            if (_aWs->availableForWriteAll())
            {
                _aWs->textAll(serialResponse);
            }

            int responseLength = serialResponse.length();
            if (incompleteResponse)
            {
                lineStartPos = responseLength;
            }
            else
            {
                // lastReceivedResponse = serialResponse;
                lineStartPos = 0;
                serialResponse = "";
            }
        }
    }
}

/* send a command via Serial, only if printer ACK-ed the last command */
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
    switch (_state)
    {
    case PH_STATE_NC:
        detectPrinter();
        break;
    case PH_STATE_CONNECTED:

        break;
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
            writeProgress(_estCompPrc);
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
    // if (_printerConnected)
    // {
        processSerialRx();
    // }
}