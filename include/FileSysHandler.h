#ifndef FILESYS_HANDLER_h
#define FILESYS_HANDLER_h

#include "Config.h"

#include <FS.h>
#if (ON == ENABLE_SD_CARD)
#include <SPI.h>
#include <SD.h>
#endif
#include <SPIFFS.h>

enum FSHandlerState
{
    NOT_INIT,
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
    size_t getSpiffsFreeBytes()
    {
        return (SPIFFS.totalBytes() - SPIFFS.usedBytes());
    }
    size_t getSdFreeBytes()
    {
        #if (ON == ENABLE_SD_CARD)
        return (SD.totalBytes() - SD.usedBytes());
        #else
        return 0;
        #endif
    }

    FSHandlerState getWriteState() { return _state; };
    FSStorageType getStorageType() { return _storageType; };
    uint8_t maxPathLen() { return _pathMaxLen; };
    String jsonifyDir(String dir, String ext);
    String jsonifyDir(String ext)
    {
        String dir;
        if(_storageType == FS_SPIFFS)
        {
            dir = "/";
        }
        else
        {
            dir = "/gcode";
        }

        return jsonifyDir(dir, ext);
    };

    void listDir(const char *dirname, uint8_t levels);
    void writeBytes(String &path, const uint8_t *data, size_t len);
    bool exists(String &path)
    {
        return _FSRoot->exists(path);
    };

    bool remove(String &path)
    {
        return _FSRoot->remove(path);
    };

    File openFile(String &path, const char *mode)
    {
        String filePath = path;

        if ((_storageType == FS_SPIFFS) && (path.length() > _pathMaxLen))
        {
            filePath = "/upld.gcode";
        }
        if(mode && *mode == 'w')
        {
            remove(filePath);
        }

        return _FSRoot->open(filePath, mode);
    };
};

extern FSHandler fileHandler;

#endif /* FILESYS_HANDLER_h */