#ifndef PRINT_HANDLER_h
#define PRINT_HANDLER_h

#include <FS.h>
#include <WString.h>

#include "GCode_Queue.h"

enum PrintState
{
    PS_IDLE,
    PS_PRINT_REQ,
    PS_PRINTING,
    PS_ABORT_REQ,
    PS_CANCELED        
};

class PrintHandler
{
private:
    File _file;
    PrintState _state;
    MsgQueue _commands;
    size_t _fileSize;
    size_t _prcSize;
    uint8_t _estCompPrc;
    HardwareSerial _serial;
    static const uint8_t COMMENT_CHAR = ';';
    static const char EOL_CHAR = '\n';
    static const uint8_t CMD_SLOTS = 3;

    void sendNow();
    void toLcd(String& text) { _commands.push("M117 " + text); };    
    void writeProgress(uint8_t prc) { _commands.push("M117 Completed " + (String)prc + "%"); };
    void parseFile();
    void sendToPrinter();
    bool pushCommand(String& cmd)
    {
        if(!_commands.isFull())
        {
            _commands.push(cmd);
            return true;
        }
        return false;
    };
public:
    uint8_t getState() { return _estCompPrc; };
    void begin(File& file, HardwareSerial& port, uint32_t baud);
    void loop();
};


#endif