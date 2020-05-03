#include "GCode_Queue.h"

String MsgQueue::pop()
{
	uint8_t next = 0;
    String ret;

    /* buffer apperas empty */
    if (_head == _tail)
    {
        return "";
    }
    /* next will be the tail after this pop */
    next = _tail + 1;
    if(next >=MAX_QUEUE_LENGTH)
    {
        next = 0;
    }
    
    ret = _msgQueue[_tail];
    /* update the tail */
    _tail = next;
    _full = false;

    return ret;
}

bool MsgQueue::push(const String& msg)
{
    uint32_t next = _head + 1;
    /* we are at top, circle to first element */
    if (next >= MAX_QUEUE_LENGTH)
    {
        next = 0;
    }
    /* buffer full, and no element was freed */
    if (next == _tail) 
    {
        _full = true;
        /* return error */
        return false;
    }
    /* add the data to the buffer */
    _msgQueue[_head] = msg;
    /* advance the head */
    _head = next;
    
    return true;
}