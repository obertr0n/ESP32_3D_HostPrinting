#ifndef MESSAGE_QUEUE_h
#define MESSAGE_QUEUE_h

#include <WString.h>
#include <inttypes.h>
#include "Config.h"

template <class T> class MessageQueue
{
private:
    T _msgQueue[HP_CMD_QUEUE_SIZE];
    uint32_t _tail;
    uint32_t _head;
    bool _full;

public:
    MessageQueue()
    {
        _tail = 0;
        _head = 0;
        _full = false;
    };
    bool push(T const& elem);
    void pop();
    T front();

    uint32_t capacity() 
    { 
        return HP_CMD_QUEUE_SIZE; 
    };

    uint32_t size() 
    { 
        return HP_CMD_QUEUE_SIZE - slots(); 
    };

    bool isfull() 
    { 
        return _full; 
    };

    bool isempty() 
    { 
        return (!_full && (_tail == _head)); 
    };

    void clear() 
    {
        _tail = 0;
        _head = 0;
    };

    uint32_t slots()
    {
        uint32_t freePos = 0;

        if(isempty())
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

template <class T>
void MessageQueue<T>::pop()
{
	uint32_t next = 0;

    /* next will be the tail after this pop */
    next = _tail + 1;
    if(next >= HP_CMD_QUEUE_SIZE)
    {
        next = 0;
    }
    /* update the tail */
    _tail = next;
    _full = false;
}

template <class T>
T MessageQueue<T>::front()
{
    T ret;
    if(!isempty())
    {
        ret = _msgQueue[_tail];
    }
    
    return ret;
}

template <class T>
bool MessageQueue<T>::push(T const& elem)
{
    uint32_t next = _head + 1;
    /* we are at top, circle to first element */
    if (next >= HP_CMD_QUEUE_SIZE)
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
    _msgQueue[_head] = elem;
    /* advance the head */
    _head = next;
    
    return true;
}

#endif /* MESSAGE_QUEUE_h */