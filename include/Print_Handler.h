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

class PrintHandler
{
private:
    File _file;
    PrintState _state;
    MsgQueue _commands;
    MsgQueue _printerMsg;
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
    bool _ackRcv;
    bool _abortReq;

    uint8_t _extruderCnt;
    String _printerName;

    String _bedTempPrest;
    String _bedTemp;
    String _extTempPreset[20];
    String _extTemp[20];

    String _serialReply;

    static const uint16_t BAUD_RATES_COUNT = 1;
    const uint32_t BAUD_RATES[BAUD_RATES_COUNT] = {115200};

    // static const uint16_t BAUD_RATES_COUNT = 9;
    // const uint32_t BAUD_RATES[BAUD_RATES_COUNT] = {2400, 9600, 19200, 38400, 57600, 115200, 250000 , 500000, 1000000};
    static const char COMMENT_CHAR = ';';
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
    static const String OK_STR;

    void preBuffer();
    void toLcd(String &text)
    {
        _commands.push("M117 " + text);
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
            _commands.push("M73 P" + (String)prc);
            _prgTout = millis() + TOUT_PROGRESS;
            _prevCmd = PH_CMD_M73;
        }
    };

    void updateWSState();
    void resetCommTimeout() { _commTout = millis() + TOUT_COMM; };
    bool parseFile();
    String parseLine(String &line);
    void appendLine(String &line);
    void processSerialRx();
    void processSerialTx();
    void detectPrinter();

    bool parseTemp(const String &line);
    bool parseFwinfo(const String &line);
    bool parse503(const String &line);
    bool parseM115(const String &line);

public:
    PrintHandler(HardwareSerial *port)
    {
        _serial = port;
        _estCompPrc = 0;
        _prgTout = 0;
        _wstxTout = 0;
        
        _printCompleted = false;
        _printCanceled = false;
        _printStarted = false;
        _printerConnected = false;
        _printRequested = false;
        _abortReq = false;
        _ackRcv = false;
        
        _state = PH_STATE_NC;
    };
    bool appendCommand(String command, bool isMaster=false)
    {
        /* only add a command if printer is connected */
        if (_printerConnected)
        {
            /* special case where a master command is requested */
            if(!isPrinting() || isMaster)
            {
                return _commands.push(command);
            }
        }
        else
        {
            return false;
        }
    }

    void abortPrint()
    {
        _abortReq = true;
    }

    bool isPrinting() { return (_state == PH_STATE_PRINT_REQ || _state == PH_STATE_PRINTING); }
    void startPrint(File &file)
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
        while (!_commands.isEmpty())
        {
            _serial->print("CMD: ");
            _serial->println(_commands.pop());
        }
    }
};

#endif