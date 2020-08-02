#ifndef GCODE_QUEUE_h
#define GCODE_QUEUE_h

#include <WString.h>
#include <inttypes.h>
#include "HP_Config.h"

struct GCodeCmd
{
    String command;
    uint32_t line;
};

class MsgQueue
{
private:
    GCodeCmd _msgQueue[HP_CMD_QUEUE_SIZE];
    uint32_t _tail;
    uint32_t _head;
    bool _full;

public:
    MsgQueue()
    {
        _tail = 0;
        _head = 0;
        _full = false;
    };
    bool push(const GCodeCmd& cmd);
    GCodeCmd pop();
    String peek();
    uint32_t maxSize() 
    { 
        return HP_CMD_QUEUE_SIZE; 
    };
    uint32_t ocuppiedSlots() 
    { 
        return HP_CMD_QUEUE_SIZE - freeSlots(); 
    };
    bool isFull() 
    { 
        return _full; 
    };
    bool isEmpty() 
    { 
        return (!_full && (_tail == _head)); 
    };
    void clear() 
    {
        _tail = 0;
        _head = 0;
    }
    uint32_t freeSlots()
    {
        uint32_t freePos = 0;

        if(isEmpty())
        {
            freePos = HP_CMD_QUEUE_SIZE;
        }
        else if(!_full)
        {
            if(_head > _tail)
            {
                freePos = HP_CMD_QUEUE_SIZE - _head;
            }
            else
            {
                freePos = HP_CMD_QUEUE_SIZE + _head - _tail;
            }            
        }  
        return freePos;
    };
};


#endif