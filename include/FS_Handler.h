#ifndef FS_HANDLER_h
#define FS_HANDLER_h

#include <FS.h>

enum FSHandlerState
{
    NO_INIT,
    IDLE,
    WRITING,
    ERROR,
    SUCCESS
};

enum FSStorageType
{
    FS_SPIFFS,
    FS_SD,
    FS_SD_SPIFFS,
    FS_NONE
};

class FSHandler
{
private:
    FS *_FSRoot;
    uint8_t _pathMaxLen;
    FSStorageType _storageType;
    FSHandlerState _state;

public:
    bool begin();
    FSHandlerState getWriteState() { return _state; };
    FSStorageType getStorageType() { return _storageType; }
    uint8_t maxPathLen() { return _pathMaxLen; }
    String jsonifyDir(String dir, String ext);
    bool remove(String path)
    {
        if (_FSRoot->exists(path))
        {
            _FSRoot->remove(path);
            return true;
        }
        return false;
    };
    void listDir(const char *dirname, uint8_t levels);
    void writeBytes(String &path, const uint8_t *data, size_t len);
    bool exists(String &path)
    {
        return _FSRoot->exists(path);
    }
    File openFile(String &path, const char *mode)
    {
        return _FSRoot->open(path, mode);
    };
};

#endif