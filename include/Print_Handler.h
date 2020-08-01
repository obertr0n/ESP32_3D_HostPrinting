#ifndef PRINT_HANDLER_h
#define PRINT_HANDLER_h

#include <FS.h>
#include <WString.h>
#include <AsyncWebSocket.h>

#include "GCode_Queue.h"

enum PrintState
{
    PH_STATE_NC,
    PH_STATE_CONNECTED,
    PH_STATE_IDLE,
    PH_STATE_PRINT_REQ,
    PH_STATE_PRINTING,
    PH_STATE_ABORT_REQ,
    PH_STATE_CANCELED
};

enum TrackedCommands
{
    PH_CMD_M115,
    PH_CMD_M117,
    PH_CMD_M73,

    PH_CMD_OTHER
};

enum AckState
{
    ACK_DEFAULT,
    ACK_OK,
    ACK_WAIT,
    ACK_BUSY,
    ACK_RESEND
};

class PrintHandler
{
private:
    File _file;
    PrintState _state;
    MsgQueue _sentPrintCmd;

    GCodeCmd _storedPrintCmd[HP_MAX_SAVED_CMD];
    uint32_t _storedCmdIdx;

    HardwareSerial *_serial;
    AsyncWebSocket *_aWs;
    TrackedCommands _prevCmd;

    size_t _fileSize;
    uint8_t _estCompPrc;
    uint32_t _wstxTout;
    uint32_t _prgTout;
    uint32_t _commTout;

    bool _printerConnected;
    bool _printRequested;
    bool _printCompleted;
    bool _printCanceled;
    bool _printStarted;
    AckState _ackRcv;
    bool _abortReq;

    uint8_t _extruderCnt;
    String _printerName;

    String _bedTempPrest;
    String _bedTemp;
    String _extTempPreset[20];
    String _extTemp[20];

    String _serialReply;
    uint32_t _currentLineNo;
    uint32_t _rejectedLineNo;

    static const uint16_t BAUD_RATES_COUNT = 1;
    static const uint32_t INVALID_LINE = 0xffffffff;
    const uint32_t BAUD_RATES[BAUD_RATES_COUNT] = {115200};

    // static const uint16_t BAUD_RATES_COUNT = 9;
    // const uint32_t BAUD_RATES[BAUD_RATES_COUNT] = {2400, 9600, 19200, 38400, 57600, 115200, 250000 , 500000, 1000000};
    /* transmit every 5s the progress  */
    static const uint32_t TOUT_PROGRESS = 5 * 1000;
    /* communicaiton timeout in 3s */
    static const uint32_t TOUT_COMM = 3 * 1000;
    /* timeout for websocked transmission */
    static const uint32_t TOUT_WSTX = 3 * 100;

    static const char M_COMMAND = 'M';
    static const char G_COMMAND = 'G';
    static const char T_COMMAND = 'T';
    static const char LF_CHAR = '\n';
    static const char CR_CHAR = '\r';

    static const String FIRMWARE_NAME_STR;
    static const String EXTRUDER_CNT_STR;

    void preBuffer();
    
    void toLcd(String &text)
    {
        addCommand("M117 " + text, true, false);
        _prevCmd = PH_CMD_M117;
    };
    void write(String &text)
    {
        if (_serial->availableForWrite())
        {
            _serial->println(text);
        }
    }
    bool isTempReply(String &reply)
    {
        return ((reply.indexOf("T:") > -1) && (reply.indexOf("B:") > -1));
    }
    bool isMoveReply(String &reply)
    {
        return ((reply.indexOf("X:") > -1) && (reply.indexOf("Y:") > -1));
    }
    void sendM115()
    {
        String m115 = "M115";
        send(m115);
        _prevCmd = PH_CMD_M115;
    }

    void writeProgress(uint8_t prc)
    {
        if ((millis() >= _prgTout) && (_prevCmd != PH_CMD_M73))
        {
            addCommand("M73 P" + (String)prc);
            _prgTout = millis() + TOUT_PROGRESS;
            _prevCmd = PH_CMD_M73;
        }
    };

    void detectPrinter();
    void updateWSState();
    void resetCommTimeout() { _commTout = millis() + TOUT_COMM; };
    bool parseFile();
    void addLine(String &line);
    /* simple circular buffer */
    void storeSentCmd(GCodeCmd& cmd)
    {
        _storedCmdIdx = _storedCmdIdx + 1;
        _storedCmdIdx %= HP_MAX_SAVED_CMD;

        _storedPrintCmd[_storedCmdIdx] = cmd; 
    }
    /* search for and entry with the same line number
       empty string represents not found
     */
    String getStoredCmd(uint32_t no)
    {
        for(uint32_t idx = _storedCmdIdx; idx >= 0; idx--)
        {
            if(_storedPrintCmd[idx].line == no)
            {
                return _storedPrintCmd[idx].command;
            }
        }
        
        return "";
    }
    void processSerialRx();
    void processSerialTx();

    bool parseTemp(const String &line);
    bool parseFwinfo(const String &line);
    bool parse503(const String &line);
    bool parseM115(const String &line);
    void startPrint();

public:
    PrintHandler(HardwareSerial *port)
    {
        _serial = port;
        _estCompPrc = 0;
        _prgTout = 0;
        _wstxTout = 0;
        _storedCmdIdx = 0;
        
        _printCompleted = false;
        _printCanceled = false;
        _printStarted = false;
        _printerConnected = false;
        _printRequested = false;
        _abortReq = false;
        _ackRcv = ACK_BUSY;
        
        _state = PH_STATE_NC;
    };

    bool addCommand(String& command, bool master=true, bool chksum=true)
    {
        uint8_t checksum;
        String cmd;
        GCodeCmd msg;
         /* only add a command if printer is connected */
        if (_printerConnected)
        {
            /* special case where a master command is requested */
            if(!isPrinting() || master)
            {
                if(chksum)
                {
                    /* compute checksum */
                    checksum = 0U;
                    for(uint32_t i = 0U; i < command.length(); i++)
                    {
                        checksum ^= command[i];
                    }
                    cmd = 'N' + _currentLineNo + command + '*' + checksum;
                }
                else
                {
                    cmd = command;
                }
                msg.command = cmd;
                msg.line = _currentLineNo;
                
                return _sentPrintCmd.push(msg);
            }
        }
        return false;    
    }

    void abortPrint()
    {
        _abortReq = true;
    }

    bool isPrinting() { return (_state == PH_STATE_PRINT_REQ || _state == PH_STATE_PRINTING); }
    void requestPrint(File &file)
    {
        _file = file;
        _fileSize = _file.size();
        _printRequested = true;
    };
    void begin(AsyncWebSocket *ws);
    void loopTx();
    void loopRx();
    bool send(String &command);

    String getState()
    {
        String stateStr;

        switch (_state)
        {
        case PH_STATE_NC:
            stateStr = "Not Connected";
            break;
        case PH_STATE_IDLE:
        case PH_STATE_CONNECTED:
            stateStr = "Connected";
            break;
        case PH_STATE_PRINT_REQ:
        case PH_STATE_PRINTING:
            stateStr = "Printing";
            break;
        case PH_STATE_ABORT_REQ:
        case PH_STATE_CANCELED:
            stateStr = "Print Cancelled";
            break;
        default:
            stateStr = "Unknown";
            break;
        }
        return stateStr;
    };

    void printbuff()
    {
        while (!_sentPrintCmd.isEmpty())
        {
            _serial->print("CMD: ");
            _serial->println(_sentPrintCmd.pop());
        }
    }
};

#endif