#include "PrintHandler.h"
#include "WebPrintServer.h"

#include "Log.h"
#include "Util.h"

PrintHandler printHandler;

void PrintHandler::begin()
{
    _state = PH_STATE_DEFAULT;
}

void PrintHandler::updateWSState()
{
    String reply;
    String outStr = "{\"st\":\"";
    outStr += getState();
    if (!isPrinting())
    {
        reply = gcodeHost.getSerialReply();
        if(reply.length() > 0)
        {
            outStr += "\",\"rp\":\"" + reply;
            outStr.replace("\n", "\\n");
        }
    }
    else
    {
        outStr += "\",\"pc\":\"" + gcodeHost.getCplPerc();
    }
    outStr += "\"}";
    webServer.write(outStr);
}

void PrintHandler::processSM()
{
    switch(_state)
    {
        case PH_STATE_DEFAULT:
            _state = PH_STATE_NC;
        break;
        case PH_STATE_NC:
            gcodeHost.attemptConnect();
            _state = PH_STATE_AC;
        break;
        case PH_STATE_AC:
            if(gcodeHost.isConnected())
            {
                _state = PH_STATE_CONNECTED;
            }
        break;
        case PH_STATE_CONNECTED:
            if(!gcodeHost.isConnected())
            {
                _state = PH_STATE_NC;
            }
            else
            {
                _state = PH_STATE_IDLE;
            }
            
        break;
        case PH_STATE_IDLE:
        break;
        case PH_STATE_PRINT_REQ:
            _state = PH_STATE_PRINTING;
        break;
        case PH_STATE_PRINTING:
            if(_reqAbort)
            {            
                _state = PH_STATE_ABORT_REQ;
                gcodeHost.requestAbort();
            }
        break;
        case PH_STATE_ABORT_REQ:
            _state = PH_STATE_IDLE;
        break;
        default:
        break;
    }
}

