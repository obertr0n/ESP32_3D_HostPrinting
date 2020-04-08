#ifndef PRINT_HANDLER_h
#define PRINT_HANDLER_h

#include <FS.h>
#include <WString.h>
#include <AsyncWebSocket.h>

#include "GCode_Queue.h"

enum PrintState
{
    PH_STATE_IDLE,
    PH_STATE_PRINT_REQ,
    PH_STATE_PRINTING,
    PH_STATE_ABORT_REQ,
    PH_STATE_CANCELED        
};

class PrintHandler
{
private:
    File _file;
    PrintState _state;
    MsgQueue _commands;
    MsgQueue _printerMsg;
    HardwareSerial* _serial;
    AsyncWebSocket* _aWs;

    size_t _fileSize;
    // size_t _prcSize;
    uint8_t _estCompPrc;
    uint32_t _prgTout;
    bool _printCompleted;
    bool _printCanceled;
    bool _printStarted;
    bool _ackRcv;

    static const uint8_t COMMENT_CHAR = ';'; 
    /* transmit every 2s the progress  */
    static const uint32_t TOUT_PROGRESS = 2 * 1000;
    static const char M_COMMAND = 'M';
    static const char G_COMMAND = 'G';
    static const char T_COMMAND = 'T';
    static const char LF_CHAR = '\n';
    static const char CR_CHAR = '\r';

    void sendNow();
    void preBuffer();
    void toLcd(String& text) { _commands.push("M117 " + text); };    
    void writeProgress(uint8_t prc) { _commands.push("M117 Completed " + (String)prc + "%"); };
    void parseFile();
    void sendToPrinter();
    String parseLine(String& line);
    void addCommand(String& command);
    void processSerialRx();
    void processSerialTx();
    void decodePrinterMsg();
public:
    PrintHandler(HardwareSerial* port)
    {
        _serial = port;
        _state = PH_STATE_IDLE;
        _estCompPrc = 0;
        _prgTout = 0;
        _printCompleted = false;
        _printCanceled = false;
        _printStarted = false;
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
    void begin(uint32_t baud, AsyncWebSocket* ws);
    void loopTx();
    void loopRx();
    bool send(String& command);
};


#endif