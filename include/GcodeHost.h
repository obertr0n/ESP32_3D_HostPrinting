#ifndef GCODE_HOST_h
#define GCODE_HOST_h

#include "FileSysHandler.h"
#include "SerialHandler.h"
#include "Log.h"
#include "Config.h"
// #include ""

enum AckState
{
    ACK_DEFAULT,
    ACK_OK,
    ACK_WAIT,
    ACK_BUSY,
    ACK_RESEND,
    ACK_OUT_OF_SYNC
};

enum GcodeCommands
{
    GH_CMD_NONE,

    GH_CMD_M115,
    GH_CMD_M117,
    GH_CMD_M73,

    GH_CMD_OTHER
};

enum TransmitState
{
    GH_STATE_OFF,
    GH_STATE_NC,
    GH_STATE_IDLE,
    GH_STATE_PRINT_REQ,
    GH_STATE_BUFFERING,
    GH_STATE_PRINTING,
    GH_STATE_PRINT_DONE
};

class GcodeCmd
{
    public:
        String command;
        uint32_t line;
};

class GcodeHost
{
    private:
        SemaphoreHandle_t _xMutex;
        uint8_t _serialRxBuffer[HP_SERIAL_RX_BUFFER_SIZE];
        String _serialReply;
        String _rxReply;
        uint32_t _conTimeout;
        File _file;
        AckState _rxAckState;
        AckState _txAckState;
        TransmitState _txState;
        bool _connected;
        bool _printing;
        MessageQueue<GcodeCmd>* _cmdQueue;
        uint32_t _rejectedLineNo;
        uint32_t _queueLineNo;
        uint32_t _ackedLineNo;
        GcodeCmd _storedPrintCmd[HP_MAX_SAVED_CMD];
        uint32_t _storedCmdIdx;
        size_t _fileSize;
        uint8_t _estCompPrc;

        bool rxCheckAckReply(String &reply);
        bool rxCheckM115(const String &line);
        bool rxProcessReply();

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
            String m115_str = "M115";
            println(m115_str);
        }

        size_t println(String &command)
        {
            String cmd = command + "\n";
            return serialHandler.write((uint8_t*)cmd.c_str(), cmd.length());
        }
        
        void storeSentCmd(GcodeCmd &cmd);
        String getStoredCmd(uint32_t no);
        void prebufferFile();
        String computeChecksum(String& cmd);
        void queueLine(String& line);
        void parseAndQueueFile();
        bool isPrintDone();
        void popAndSendCommand();
        void txNotConnectedState();
        void txIdleState();
        void txPrintReqState();
        void txBufferingState();
        void txPrintingState();
        void txPrintDoneState();

        void resetState();
        void acquireLock()
        {
            do
            {
                ;
            }while(xSemaphoreTake(_xMutex, portMAX_DELAY) != pdPASS);
        }
        void releaseLock()
        {
            xSemaphoreGive(_xMutex);
        }
        bool queueCmd(String& command, bool master, bool chksum);
        void transmit_SM();
    public:
        GcodeHost()
        {
            _cmdQueue = new MessageQueue<GcodeCmd>();
            assert(_cmdQueue != nullptr);
        }
        ~GcodeHost()
        {
            if(_cmdQueue)
            {
                delete _cmdQueue;
            }
        }
        void begin()
        {
            _xMutex = xSemaphoreCreateMutex();
            _connected = false;
            _txState = GH_STATE_OFF;
            _conTimeout = 0;
            resetState();
        }
        void attemptConnect()
        {
            _txState = GH_STATE_NC;
        }
        bool addCommand(String& command, bool master, bool chksum)
        {
            bool ret;
            acquireLock();
            ret = queueCmd(command, master, chksum);
            releaseLock();

            return ret;
        }
        void loopTx() { transmit_SM(); }
        bool loopRx() { return rxProcessReply(); }
        bool isConnected() { return _connected; }
        void toLcd(String &text)
        {
            queueCmd("M117 " + text, true, false);
        };
        AckState getRxAckState() 
        { 
            AckState state;
            
            acquireLock();
            state = _rxAckState;
            releaseLock();

            return state; 
        }
        void setRxAckState(AckState s) 
        { 
            acquireLock();
            _rxAckState = s; 
            releaseLock();            
        }
        
        AckState getTxAckState() 
        { 
            AckState state;
            
            acquireLock();
            state = _txAckState;
            releaseLock();

            return state; 
        }
        void setTxAckState(AckState s) 
        { 
            acquireLock();
            _txAckState = s; 
            releaseLock();            
        }        
        bool requestPrint(String& filename);
        void requestAbort();
        String getSerialReply()
        {
            String reply;
            acquireLock();
            reply = _serialReply;
            _serialReply = "";
            releaseLock();

            return reply;
        }
        String getCplPerc() { return ((String)_estCompPrc); }
};

extern GcodeHost gcodeHost;

#endif