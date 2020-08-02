#include "Print_Handler.h"
#include "HP_Config.h"
#include "HP_Util.h"

const String PrintHandler::FIRMWARE_NAME_STR = "FIRMWARE_NAME:";
const String PrintHandler::EXTRUDER_CNT_STR = "EXTRUDER_COUNT:";

void PrintHandler::begin(AsyncWebSocket *ws)
{
    _aWs = ws;
    _serial->begin(BAUD_RATES[0]);
}

void PrintHandler::preBuffer()
{
    String line;
    while (_sentPrintCmd.freeSlots() > HP_CMD_SLOTS)
    {
        if (_file.available() > 0)
        {
            line = _file.readStringUntil(LF_CHAR);
            addLine(line);
        }
    }
}

bool PrintHandler::parseFile()
{
    String line;
    size_t bytesAvail = 0;

    bytesAvail = _file.available();
    if (bytesAvail > 0)
    {
        if (_sentPrintCmd.freeSlots() > HP_CMD_SLOTS)
        {
            line = _file.readStringUntil(LF_CHAR);
            
            addLine(line);
            _estCompPrc = 100U - (bytesAvail * 100U / _fileSize);
        }
        return true;
    }
    else
    {
        _file.close();
        return false;
    }
}

void PrintHandler::addLine(String& line)
{
    line.trim();
    const int commentIdx = line.indexOf(';');
    
    if(commentIdx > -1)
    {
        line = line.substring(0, commentIdx);
        line.trim();
        if(line.length()  > 2)
        {
            queueCommand(line);
        }
    }
    else if(line.length()  > 2)
    {
        queueCommand(line);
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
            int ok_pos = line.indexOf("ok");
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
    static String l_serialReply;
    const String v_resend_str = "Resend: ";
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
                    _state = PH_STATE_CONNECTED;
                    _ackRcv = ACK_OK;
                    _printerConnected = true;

                    String ip = util_getIP();
                    toLcd("Connect: " + ip);
                }
                else
                {
                    replyFound = false;
                }
            }
            else
            {
                if (l_serialReply.startsWith("echo:busy"))
                {
                    _ackRcv = ACK_BUSY;
                }
                // else if (l_serialReply.startsWith("Error"))
                // {
                //     _printCanceled = true;
                // }
                else if (l_serialReply.indexOf(v_resend_str) > -1)
                {
                    _serial->print("Org Str: ");
                    _serial->println(l_serialReply);
                    int strOffset = l_serialReply.indexOf(v_resend_str) + 8;
                    _serial->print("Offset: ");
                    _serial->println(strOffset);

                    String lineNum = l_serialReply.substring(strOffset, l_serialReply.indexOf('\n'));

                    _serial->print("Resend line: ");
                    _serial->println(lineNum);

                    _rejectedLineNo = lineNum.toInt();

                    _ackRcv = ACK_RESEND;
                }
                else if (isMoveReply(l_serialReply) || isTempReply(l_serialReply))
                {
                    resetCommTimeout();
                }
                else if (l_serialReply.indexOf("ok") > -1)
                {
                    switch (_ackRcv)
                    {
                        case ACK_OUT_OF_SYNC:
                            _ackRcv = ACK_RESEND;
                        break;
                        case ACK_DEFAULT:
                        case ACK_WAIT:
                        case ACK_BUSY:
                            _ackRcv = ACK_OK;
                        break;
                        case ACK_RESEND:
                        default:

                        break;

                    }
                    resetCommTimeout();
                }
                else
                {
                    replyFound = false;
                }                
            }            
            if (replyFound)
            {
                _serialReply = l_serialReply;
                l_serialReply = "";
            }
        }
    }
}
static AckState prev_AckRcv;
/* send a command via Serial, only if printer ACK-ed the last command */
void PrintHandler::processSerialTx()
{
    String cmdToSend;
    GCodeCmd cmd;
    // if(prev_AckRcv != _ackRcv)
    // {
        // _serial->println(">a " + (String)_ackRcv);
    //     prev_AckRcv = _ackRcv;
    // }
    /* previous command was OK */
    if(ACK_OK == _ackRcv)
    {
        cmd = _sentPrintCmd.pop();
        cmdToSend = cmd.command;
        if(isPrinting())
        {
            /* let's store the last sent line */
            storeSentCmd(cmd);
        }
    }
    else if(ACK_RESEND == _ackRcv && _rejectedLineNo != INVALID_LINE)
    {
        /* we need to resend a previous command */
        cmdToSend = getStoredCmd(_rejectedLineNo);
        _rejectedLineNo += 1U;
        /* if we are out of sync by only 1 command than no resending next cycle */
        if(_rejectedLineNo == _currentLineNo)
        {
            _rejectedLineNo = INVALID_LINE;
        }
        else
        {
            // we are STILL out of sync with the printer, try and recover
            _ackRcv = ACK_OUT_OF_SYNC;
        }
    }

    if (cmdToSend != "")
    {
        write(cmdToSend);
        /* reset it only when the transmission is done? */
        _ackRcv = ACK_DEFAULT;
        _prevCmd = PH_CMD_OTHER;
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
        startPrint();
        _state = PH_STATE_PRINTING;
        break;

    case PH_STATE_PRINTING:
        if(!_abortReq && !_printCanceled)
        {            
            /* file still has data and the command buffer is empty */
            if((parseFile() == false) && (_sentPrintCmd.isEmpty() == true))
            {             
                _printStarted = false;
                _printCompleted = true;
            }
            if (!_printCompleted)
            {
                processSerialTx();
            }
            else
            {
                _state = PH_STATE_IDLE;
            }
        }        
        else
        {
            /* E0 OFF, Bed OFFF, Move Z10 mm up, WAIT FOR USER */
            // write("M112");
            String message = "Print aborted. Reset machine!";
            toLcd(message);

            _sentPrintCmd.clear();
            _state = PH_STATE_CANCELED;
        }
    break;
    case PH_STATE_CANCELED:
        _printCanceled = false;
        _abortReq = false;
        _printCompleted = false;
        _printStarted = false;
        _printRequested = false;
        _storedCmdIdx = 0;
        _state = PH_STATE_IDLE;
    break;
    default:
        break;
    }
}

void PrintHandler::startPrint()
{
    String command;
    _sentPrintCmd.clear();
    _printStarted = true;
    _printCompleted = false;
    _ackRcv = ACK_OK;

    // preBuffer();

    /* reset the line number and send M110 to the printer */
    _currentLineNo = 0U;
    _rejectedLineNo = 0U;

    command = "M110 N0*35";    
    queueCommand(command, true, false);

}

void PrintHandler::loopRx()
{
    processSerialRx();
    updateWSState();
}