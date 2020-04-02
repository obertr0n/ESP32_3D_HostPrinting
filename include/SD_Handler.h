#ifndef SD_HANDLER_h
#define SD_HANDLER_h

#include <FS.h>

enum SdHandlerState
{
    NO_INIT,
    IDLE,
    WRITING,
    ERROR,
    SUCCESS
};

class SdHandler
{
private:
    FS* _SDRoot;
    SdHandlerState _state;

public:
    bool begin();
    SdHandlerState getState() { return _state; };
    String jsonifyDir(String dir, String ext);
    bool remove(String path)
    {
        if(_SDRoot->exists(path))
        {
            _SDRoot->remove(path);
            return true;
        }
        return false;
    };
    void listDir(const char* dirname, uint8_t levels);
    void writeBytes(String& path, const uint8_t* data, size_t len);
    
    File openFile(String& path, const char* mode)
    {
        return _SDRoot->open(path, mode);
    };
};

#endif