#include "Print_Handler.h"
#include "HP_Config.h"

const String PrintHandler::FIRMWARE_NAME_STR = "FIRMWARE_NAME:";
const String PrintHandler::EXTRUDER_CNT_STR = "EXTRUDER_COUNT:";
const String PrintHandler::OK_STR = "ok";

void PrintHandler::begin(AsyncWebSocket *ws)
{
    _aWs = ws;
    _serial->begin(BAUD_RATES[0]);
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

    if ((!_printerConnected) && (millis() >= _commTout))
    {
        _serial->updateBaudRate(BAUD_RATES[baudIndex]);
        sendM115();

        baudIndex += 1;
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
    int t_pos = line.indexOf("T:");
    if (t_pos >= 0)
    {
        int tslh_pos = line.indexOf("/", t_pos);
        if (tslh_pos >= 0)
        {
            _extTemp[0] = line.substring(t_pos + 2, tslh_pos);
        }
        int b_pos = line.indexOf("B:");
        if (b_pos >= 0)
        {
            _extTempPreset[0] = line.substring(tslh_pos + 1, b_pos);
            int bslh_pos = line.indexOf(" /", b_pos);
            if (bslh_pos >= 0)
            {
                _bedTemp = line.substring(b_pos + 2, bslh_pos);
                int at_pos = line.indexOf(" @");
                if (at_pos >= 0)
                {
                    _bedTempPrest = line.substring(bslh_pos + 2, at_pos);
                    return true;
                }
            }
        }
    }

    return false;
}

bool PrintHandler::parse503(const String &line)
{
    _serial->println("parse503");
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
    int fwname_pos = line.indexOf(FIRMWARE_NAME_STR);
    if (fwname_pos >= 0)
    {
        fwname_pos += FIRMWARE_NAME_STR.length();
        String fwName = line.substring(fwname_pos, fwname_pos + 7);
        int extruderCnt_pos = line.indexOf(EXTRUDER_CNT_STR);
        if (extruderCnt_pos >= 0)
        {
            extruderCnt_pos += EXTRUDER_CNT_STR.length();
            String extCnt = line.substring(extruderCnt_pos, extruderCnt_pos + 1);
            int ok_pos = line.indexOf(OK_STR);
            if (ok_pos >= 0)
            {
                return true;
            }
        }
    }
    return false;
}

void PrintHandler::updateWSState()
{
    String outStr;
    if(millis() > _wstxTout)
    {
        outStr = "PS=" + getState();
        outStr += "PG=" + (String)_estCompPrc;
        outStr += "PR=" + _serialReply;
                    
        // transmit back what we received
        if (_aWs->availableForWriteAll())
        {
            _aWs->textAll(outStr);
        }
        _serialReply = "";
        _wstxTout = millis() + TOUT_WSTX;
    }
}

void PrintHandler::processSerialRx()
{
    static int startLine = 0;
    static String l_serialReply;

    while (_serial->available())
    {
        char c = (char)_serial->read();
        l_serialReply += c;

        if (c == '\n')
        {
            bool replyFound = true;
            if (PH_CMD_M115 == _prevCmd && _state == PH_STATE_NC)
            {
                if (parseM115(l_serialReply))
                {
                    String conn = "ESP32 Connected: " + WiFi.localIP();
                    _state = PH_STATE_CONNECTED;
                    toLcd(conn);
                    _printerConnected = true;
                }
                else
                {
                    replyFound = false;
                }
            }
            else
            {
                if (l_serialReply.indexOf(OK_STR) > -1)
                {
                    _ackRcv = true;
                }
                else if (isMoveReply(l_serialReply) || isTempReply(l_serialReply))
                {
                    _ackRcv = true;
                    resetCommTimeout();
                }
                else if (l_serialReply.startsWith("echo:busy"))
                {
                    _ackRcv = false;
                }
                else if (l_serialReply.startsWith("Error:"))
                {
                    _ackRcv = false;
                    _printCanceled = true;
                }
                else
                {
                    replyFound = false;
                }                
            }

            int replyLen = l_serialReply.length();
            
            if (replyFound)
            {

                replyLen = 0;
                _serialReply = l_serialReply;
                l_serialReply = "";            
            }
            else
            {
                startLine = replyLen;
            }
        }
    }
}

/* send a command via Serial, only if printer ACK-ed the last command */
void PrintHandler::processSerialTx()
{
    String printerCommand;
    printerCommand = _commands.pop();
    if (printerCommand != "")
    {
        write(printerCommand);
        /* reset it only when the transmission is done? */
        _ackRcv = false;
        _prevCmd = PH_CMD_OTHER;
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
        _prevCmd = PH_CMD_OTHER;
        _state = PH_STATE_IDLE;
        break;
    case PH_STATE_IDLE:
        if (_printRequested)
        {
            _state = PH_STATE_PRINT_REQ;
            _printRequested = false;
        }
        else
        {
            processSerialTx();
        }
        break;
    case PH_STATE_PRINT_REQ:
        _commands.clear();
        _printStarted = true;
        _printCompleted = false;
        // preBuffer();
        _ackRcv = true;
        _state = PH_STATE_PRINTING;
        break;

    case PH_STATE_PRINTING:
        if(!_abortReq && !_printCanceled)
        {
            parseFile();
            if(_commands.isEmpty())
            {                    
                _printStarted = false;
                _printCompleted = true;
            }
            if (!_printCompleted)
            {
                // writeProgress(_estCompPrc);
                if (_ackRcv)
                {
                    processSerialTx();
                }
            }
            else
            {
                _state = PH_STATE_IDLE;
            }
        }        
        else
        {
            _state = PH_STATE_ABORT_REQ;
        }
        break;
    case PH_STATE_ABORT_REQ:
    {        
        String cancel = "Print aborted";
        String command = "M0";

        send(command);
        command = "G1 Z10";
        send(command);
        toLcd(cancel);

        _commands.clear();
        _state = PH_STATE_CANCELED;
    }
    break;
    case PH_STATE_CANCELED:
    {
        _printCanceled = false;
        _abortReq = false;
        _printCompleted = false;
        _printStarted = false;
        _printRequested = false;

        _state = PH_STATE_IDLE;
    }
    break;
    default:
        break;
    }
}

void PrintHandler::loopRx()
{
    processSerialRx();
    updateWSState();
}