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
    PH_M115,
    PH_M117,
    PH_M73,

    PH_OTHER
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

    size_t _fileSize;
    // size_t _prcSize;
    uint8_t _estCompPrc;
    uint32_t _prgTout;
    uint32_t _commTout;
    bool _printerConnected;
    bool _printCompleted;
    bool _printCanceled;
    bool _printStarted;
    bool _ackRcv;
    TrackedCommands _prevCmd;

    static const uint16_t BAUD_RATES_COUNT = 9;
    const uint32_t BAUD_RATES[BAUD_RATES_COUNT] = {2400, 9600, 19200, 38400, 57600, 115200, 250000, 500000, 1000000};
    static const uint8_t COMMENT_CHAR = ';';
    /* transmit every 5s the progress  */
    static const uint32_t TOUT_PROGRESS = 5 * 1000;
    static const uint32_t TOUT_COMM = 2500;
    static const char M_COMMAND = 'M';
    static const char G_COMMAND = 'G';
    static const char T_COMMAND = 'T';
    static const char LF_CHAR = '\n';
    static const char CR_CHAR = '\r';

    void preBuffer();
    void toLcd(String &text) { _commands.push("M117 " + text); };
    void write(String &text)
    {
        if (_serial->availableForWrite())
        {
            _serial->println(text);
        }
    }
    void sendM115() 
    { 
        String m115 = "M115"; 
        send(m115);
        _prevCmd = PH_M115;  
    }
    void writeProgress(uint8_t prc) 
    {
        if ((millis() >= _prgTout) && (_prevCmd != PH_M73))
        {
            _commands.push("M73 P" + (String)prc);               
            _prgTout = millis() + TOUT_PROGRESS;
            _prevCmd = PH_M73;
        }
    };
    void resetCommTimeout() { _commTout = millis() + TOUT_COMM; };
    void parseFile();
    void sendToPrinter();
    String parseLine(String &line);
    void addCommand(String &command);
    void processSerialRx();
    void processSerialTx();
    void decodePrinterMsg();
    void detectPrinter();

    bool parseTemp(const String &line);
    bool parseFwinfo(const String &line);
    bool parse503(const String &line);
    bool parseM115(const String &line);

public:
    PrintHandler(HardwareSerial *port)
    {
        _serial = port;
        _state = PH_STATE_NC;
        _estCompPrc = 0;
        _prgTout = 0;
        _printCompleted = false;
        _printCanceled = false;
        _printStarted = false;
        _printerConnected = false;
        _ackRcv = false;
    };
    PrintState getState() { return _state; };
    bool isPrinting() { return (_state == PH_STATE_PRINT_REQ || _state == PH_STATE_PRINTING); }
    void startPrint(File &file)
    {
        _file = file;
        _fileSize = _file.size();
        _state = PH_STATE_PRINT_REQ;
    };
    void begin(AsyncWebSocket *ws);
    void loopTx();
    void loopRx();
    bool send(String &command);
};

#endif