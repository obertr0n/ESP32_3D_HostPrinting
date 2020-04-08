#ifndef GCODE_QUEUE_h
#define GCODE_QUEUE_h

#include <WString.h>
#include <inttypes.h>

class MsgQueue
{
private:
    static const uint32_t MAX_QUEUE_LENGTH = 32;
    String _msgQueue[MAX_QUEUE_LENGTH];
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
    bool push(String& cmd);
    bool pop(String& data);
    String peek();
    uint32_t maxSize() { return MAX_QUEUE_LENGTH; };
    uint32_t ocuppiedSlots() { return MAX_QUEUE_LENGTH - freeSlots(); };
    bool isFull() { return _full; };
    bool isEmpty() { return (!_full && (_tail == _head)); };
    uint32_t freeSlots()
    {
        uint32_t free = 0;
        if(!_full)
        {
            if(_head > _tail)
            {
                free = _head - _tail;
            }
            else
            {
                free = MAX_QUEUE_LENGTH + _head - _tail;
            }            
        }        
        return free;
    };
};

#endif