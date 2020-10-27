#include "GcodeHost.h"
#include "SerialHandler.h"

GcodeHost gcodeHost;

static const uint32_t M115_CONNECTION_TOUT = 500u;
static const uint32_t RESERVED_QUEUE_SLOTS = 3u;
const TickType_t RX_DATA_DELAY = 1 / portTICK_PERIOD_MS;

static const String RESEND_STR = "Resend: ";
static const uint32_t RESEND_STR_LEN = RESEND_STR.length();

static const String FIRMWARE_NAME_STR = "FIRMWARE_NAME:";
static const String EXTRUDER_CNT_STR = "EXTRUDER_COUNT:";
static const char LF_CHAR = '\n';
static const char CR_CHAR = '\r';
static const char COMMENT_CHAR = ';';
static const uint32_t INVALID_LINE_NUMBER = 0xffffffff;

/*
*@brief rest all the global variables and states
*/
void GcodeHost::resetState()
{
    // assume the serial is in sync
    _rxAckState = ACK_OK;
    _txAckState = ACK_OK;
    _printing = false;
    _rejectedLineNo = INVALID_LINE_NUMBER;
    _queueLineNo = 0;
    _ackedLineNo = 0;
    _storedCmdIdx = 0;
    _fileSize = 0;
    _estCompPrc = 0;
    _cmdQueue->clear();
}

/*
*@brief reads all the available data from serial and retrieve the ACK state. Timeouts after REPLY_PROCESS_TIMEOUT
*/
bool GcodeHost::rxProcessReply()
{
    bool replyFound = false;
    size_t availRx = serialHandler.available();
    /* have pending data */
    if (availRx > 0)
    {
        size_t bytesRead = 0;
        size_t readSize;
        if (availRx > HP_SERIAL_RX_BUFFER_SIZE - 1)
        {
            readSize = HP_SERIAL_RX_BUFFER_SIZE - 2;
            while (availRx)
            {
                bytesRead = serialHandler.readBytes(&_serialRxBuffer[bytesRead], readSize);
                _serialRxBuffer[bytesRead] = '\0';
                _rxReply += (const char *)_serialRxBuffer;
                availRx -= bytesRead;
            }
        }
        else
        {
            bytesRead = serialHandler.readBytes(&_serialRxBuffer[bytesRead], availRx);
            _serialRxBuffer[bytesRead] = '\0';
            _rxReply += (const char *)_serialRxBuffer;
        }
        if (!_connected)
        {
            if (rxCheckM115(_rxReply))
            {
                _connected = true;
                hp_log_printf("printer connected\n");
                replyFound = true;
            }
        }
        else
        {
            /* check if a valid reply was found */
            replyFound = rxCheckAckReply(_rxReply);
        }
        if (replyFound)
        {
            acquireLock();
            _serialReply = _rxReply;
            releaseLock();
            _rxReply = "";
        }
    }
    else
    {
        /* if no data, force quit */
        vTaskDelay(RX_DATA_DELAY);
    }
    return replyFound;
}

/*
*@brief Retrieve the ACK state from a serial reply
*@param reply String that holds the serial reply
*@return true if a reply was found */
bool GcodeHost::rxCheckAckReply(String &reply)
{
    AckState txAckState = getTxAckState(); // uses a mutex
    if (reply.indexOf("echo:busy") > -1)
    {
        _rxAckState = ACK_BUSY;
        return true;
    }
    else if (reply.indexOf(RESEND_STR) > -1)
    {
        int strOffset = reply.indexOf(RESEND_STR) + RESEND_STR_LEN;
        String lineNum = reply.substring(strOffset);
        lineNum.trim();
        hp_log_printf("Resend line: %s\n", lineNum.c_str());
        _rejectedLineNo = lineNum.toInt();

        _rxAckState = ACK_RESEND;
        if (reply.indexOf("ok") > -1)
        {
            return true;
        }
        else
        {
            return false;
        }
        
    }
    else if (reply.indexOf("ok") > -1)
    {
        /* we are in oos state, but the prev command was ok */
        if (ACK_OUT_OF_SYNC == txAckState)
        {
            hp_log_printf("Ok, ACK_RESEND\n");
            _rxAckState = ACK_RESEND;
        }
        else
        {
            // hp_log_printf("Ok, ACK_OK\n");
            _rxAckState = ACK_OK;
        }
        return true;
    }
    else
    {
        return false;
    }
}

/*
*@brief Check if the reply received is as result of a M115 command
*@param reply the serial reply to be checked
*@return true if previous command was a M115
*/
bool GcodeHost::rxCheckM115(const String &reply)
{
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
Cap:CHAMBER_TEMPERATURE:0
ok
*/
    int fwname_pos = reply.indexOf(FIRMWARE_NAME_STR);
    if (fwname_pos >= 0)
    {
        fwname_pos += FIRMWARE_NAME_STR.length();
        String fwName = reply.substring(fwname_pos, fwname_pos + 7);
        int extruderCnt_pos = reply.indexOf(EXTRUDER_CNT_STR);
        if (extruderCnt_pos >= 0)
        {
            extruderCnt_pos += EXTRUDER_CNT_STR.length();
            String extCnt = reply.substring(extruderCnt_pos, extruderCnt_pos + 1);
            int ok_pos = reply.indexOf("ok");
            if (ok_pos >= 0)
            {
                _rxAckState = ACK_OK;
                return true;
            }
        }
    }
    return false;
}

/*
*@brief Compute the checksum for the provided command 
*@param cmd String that hold the command
*@return initial command plus checksum
*/
String GcodeHost::computeChecksum(String &cmd)
{
    String out = "N" + (String)_queueLineNo + " " + cmd;
    uint8_t checksum = 0;
    /* compute checksum */
    checksum = 0U;
    for (uint32_t i = 0U; i < out.length(); i++)
    {
        checksum ^= out[i];
    }

    out += "*" + (String)checksum;

    return out;
}

/*
*@brief Add a command to the queue only if printer connected
*@param command reference to to a String containing the command
*@param master boolean used to specify if it can be added even when printing
*@param chksum boolean used to specify if checksum has to be added to the queued command
*@return true if success */
bool GcodeHost::queueCmd(String &command, bool master = true, bool chksum = true)
{
    GcodeCmd msg;

    /* only add a command if printer is connected */
    if (_connected)
    {
        /* special case where a master command is requested */
        if ((false == _printing) || (true == master))
        {
            if (chksum)
            {
                _queueLineNo++;
                msg.line = _queueLineNo;
                msg.command = computeChecksum(command);
            }
            else
            {
                msg.command = command;
            }

            // hp_log_printf("push: %s\n", msg.command.c_str());

            return _cmdQueue->push(msg);
        }
    }
    return false;
}

/*
*@brief Search for and entry with the same line number
*@param cmdNo line number to search for
*@return the command that was found. Empty string represents not found.
*/
String GcodeHost::getStoredCmd(uint32_t cmdNo)
{
    for (uint32_t idx = _storedCmdIdx; idx >= 0; idx--)
    {
        if (_storedPrintCmd[idx].line == cmdNo)
        {
            return _storedPrintCmd[idx].command;
        }
    }

    return "";
}

/*
*@brief Store the commands are they are sent in case of retransmit error
*@param cmd reference to the command to be stored
*/
void GcodeHost::storeSentCmd(GcodeCmd &cmd)
{
    _storedCmdIdx = _storedCmdIdx + 1;
    _storedCmdIdx %= HP_MAX_SAVED_CMD;

    _storedPrintCmd[_storedCmdIdx] = cmd;
}

/*
*@brief Buffer as many line from the file as possible
*/
void GcodeHost::prebufferFile()
{
    String line;
    while ((_cmdQueue->slots() > RESERVED_QUEUE_SLOTS) &&
           (_file.available() > 0))
    {
        line = _file.readStringUntil(LF_CHAR);
        queueLine(line);
    }
}

/*
*@brief Add a line to the message queue
*@param line reference to the line to be added
*/
void GcodeHost::queueLine(String &line)
{
    const int commentIdx = line.indexOf(';');
    //hp_log_printf("add %s\n", line.c_str());
    if (commentIdx > -1)
    {
        line = line.substring(0, commentIdx);
        line.trim();
        if (line.length() > 2)
        {
            // hp_log_printf("queue %s\n", line.c_str());
            queueCmd(line);
        }
    }
    else if (line.length() > 2)
    {
        line.trim();
        // hp_log_printf("queue %s\n", line.c_str());
        queueCmd(line);
    }
}

/*
*@brief Parse the file during printing state and queue lines
*/
void GcodeHost::parseAndQueueFile()
{
    /* continuously add commands to the queue */
    size_t bytesAvail = 0;

    bytesAvail = _file.available();
    if (bytesAvail > 0)
    {
        if (_cmdQueue->slots() > RESERVED_QUEUE_SLOTS)
        {
            String line;
            line = _file.readStringUntil(LF_CHAR);

            queueLine(line);
            _estCompPrc = 100U - (bytesAvail * 100U / _fileSize);
        }
        else
        {
            // hp_log_printf("No room\n");
        }
    }
    else
    {
        hp_log_printf("No more data\n");
        _file.close();
        _txState = GH_STATE_PRINT_DONE;
    }
}

/*
*@brief Request the start of printing a file
*@param filename the name of the file
*@return job accepted or not
*/
bool GcodeHost::requestPrint(String& filename)
{
    if (fileHandler.exists(filename))
    {
        _txState = GH_STATE_PRINT_REQ;
        _file = fileHandler.openFile(filename, FILE_READ);
        _fileSize = _file.size();

        return true;
    }
    return false;
}

/*
*@brief Request the abortion of a print. It will immediately send a Stop command to the printer
*/
void GcodeHost::requestAbort()
{
    String abortMessage = "G28";
    println(abortMessage);
    _txState = GH_STATE_PRINT_DONE;
}

/*
*@brief Get one command from queue and send it over serial
*/
void GcodeHost::popAndSendCommand()
{
    GcodeCmd cmd;
    AckState rxAckState = getRxAckState();

    if ((ACK_RESEND == rxAckState) && 
        (_rejectedLineNo != INVALID_LINE_NUMBER))
    {
        cmd.command = getStoredCmd(_rejectedLineNo);
        /* consider the \n as well */
        if((cmd.command.length() + 1) == println(cmd.command))
        {
            _rejectedLineNo++;
            setRxAckState(ACK_DEFAULT);
        }
        hp_log_printf("Reject: %d Current: %d\n", _rejectedLineNo, _ackedLineNo);

        /* if we are out of sync by only 1 command than no resending next cycle */
        if (_rejectedLineNo == _ackedLineNo)
        {
            _rejectedLineNo = INVALID_LINE_NUMBER;
            /* reset state */
            _txAckState = ACK_DEFAULT;
        }
        else
        {
            hp_log_printf("OoS\n");
            // we are STILL out of sync with the printer, try and recover
            _txAckState = ACK_OUT_OF_SYNC;
        }            
    }
    /* we received an OK from serial */
    else if(ACK_OK == rxAckState)
    {
        if(!_cmdQueue->isempty())
        {
            cmd = _cmdQueue->front();
            if(_printing)
            {
                storeSentCmd(cmd);
                _ackedLineNo++;
            }
            /* consider the \n as well */
            if(println(cmd.command) == (cmd.command.length() + 1))
            {
                /* all data sent, safe to pop */
                _cmdQueue->pop();
                setRxAckState(ACK_DEFAULT);
            }            
            /* reset state */
            _txAckState = ACK_DEFAULT;
        }
    }
}
/*
*@brief Attempts connection to printer
*/
void GcodeHost::txNotConnectedState()
{
    if((false == _connected) && (_conTimeout <= millis()))
    {
        sendM115();
        _conTimeout = millis() + M115_CONNECTION_TOUT;
    }
    else if (true == _connected)
    {
        String msg = "Connect " + Util.getIp();
        toLcd(msg);
        _txState = GH_STATE_IDLE;
    }
}

/*
*@brief Transmits commands queue by web interface
*/
void GcodeHost::txIdleState()
{
    AckState rxAckState = getRxAckState();
    /* not printing, but ACK received */
    if((false == _printing) && (rxAckState == ACK_OK))
    {
        /* still have data */
        if(false == _cmdQueue->isempty())
        {
            GcodeCmd cmd = _cmdQueue->front();
            _cmdQueue->pop();

            if(false == cmd.command.isEmpty())
            {
                println(cmd.command);
                setRxAckState(ACK_DEFAULT);
            }
        }
    }
}

/*
*@brief Handle the print requests
*/
void GcodeHost::txPrintReqState()
{
    _cmdQueue->clear();

    _printing = true;
    _txAckState = ACK_OK;
    /* reset the line number */
    _queueLineNo = 0U;
    _ackedLineNo = 0U;
    _rejectedLineNo = 0U;

    /* and send M110 to the printer */
    String startPrintCmd = "M110 N0*35";
    queueCmd(startPrintCmd, true, false);

    /* next state */
    _txState = GH_STATE_BUFFERING;
}

/*
*@brief Handle the prebuffering state
*/
void GcodeHost::txBufferingState()
{
    prebufferFile();
    /* next state */
    _txState = GH_STATE_PRINTING;
}

/*
*@brief Handle the printing state
*/
void GcodeHost::txPrintingState()
{
    parseAndQueueFile();
    popAndSendCommand();
}

/*
*@brief Handle successful or unsuccessful print completion
*/
void GcodeHost::txPrintDoneState()
{
    resetState();
    _txState = GH_STATE_IDLE;
}

/*
*@brief Tx state machine
*/
void GcodeHost::transmit_SM()
{
    switch (_txState)
    {
        case GH_STATE_NC:
            txNotConnectedState();
        break;
        case GH_STATE_IDLE:
            txIdleState();
        break;
        case GH_STATE_PRINT_REQ:
            txPrintReqState();
        break;
        case GH_STATE_BUFFERING:
            txBufferingState();
        break;
        case GH_STATE_PRINTING:
            txPrintingState();
        break;
        case GH_STATE_PRINT_DONE:
            txPrintDoneState();
        default:
        break;
    }
}