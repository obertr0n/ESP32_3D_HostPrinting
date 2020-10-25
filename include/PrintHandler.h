#ifndef PRINT_HANDLER_h
#define PRINT_HANDLER_h

#include <AsyncWebSocket.h>
#include "GcodeHost.h"
#include "Log.h"

enum PrintState
{
    PH_STATE_DEFAULT,
    PH_STATE_NC,
    PH_STATE_AC,
    PH_STATE_CONNECTED,
    PH_STATE_IDLE,
    PH_STATE_PRINT_REQ,
    PH_STATE_PRINTING,
    PH_STATE_ABORT_REQ
};

class PrintHandler
{
private:
    PrintState _state;
    bool _reqAbort;
    void detectPrinter();
    void startPrint();
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
            stateStr = "Print Cancelled";
            break;
        default:
            stateStr = "Unknown";
            break;
        }
        return stateStr;
    };
public:
    void abortPrint()
    {
        _reqAbort = true;
    }
    bool isPrinting() 
    { 
        return ((_state == PH_STATE_PRINT_REQ) || (_state == PH_STATE_PRINTING)); 
    }
    bool requestPrint(String &filename)
    {
        hp_log_printf("Print req for: %s\n", filename.c_str());
        if(gcodeHost.requestPrint(filename))
        {
            _state = PH_STATE_PRINT_REQ;
            return true;
        }
        return false;
    };
    void processSM();
    void updateWSState();
    void begin();
};

extern PrintHandler printHandler;

#endif