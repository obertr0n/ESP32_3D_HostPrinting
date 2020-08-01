#include <SD.h>
#include <SPI.h>
#include "SPIFFS.h"

#include "FS_Handler.h"
#include "HP_Config.h"

bool FSHandler::begin()
{
    /* init SPI for SD communication */
    SPI.begin(PIN_SD_CLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_SS);
    /* at the moment we don't know the FS type */
    _storageType = FS_NONE;
    _FSRoot = NULL;
    _state = ERROR;
    bool ret = false;
    if (SPIFFS.begin())
    {
        LOG_Println("SPIFFS mounted.");
        _storageType = FS_SPIFFS;
        _FSRoot = &SPIFFS;
        _state = IDLE;
        _pathMaxLen = 11;
        ret = true;
    }
    /* try and mount SD */
    if (SD.begin(PIN_SD_SS))
    {
        LOG_Println("SD Card initialized.");
        if (_storageType == FS_SPIFFS)
        {
            /* we have both available */
            _storageType = FS_SD_SPIFFS;
        }
        else
        {
            /* only SD available */
            _storageType = FS_SD;
        }
        _pathMaxLen = 255;
        _FSRoot = &SD;
        _state = IDLE;

        ret= true;
    }
    return ret;
}

void FSHandler::listDir(const char *dirname, uint8_t levels)
{
    File root = _FSRoot->open(dirname);
    if (!root)
    {
        LOG_Println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        LOG_Println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            LOG_Println("  DIR : ");
            LOG_Println(file.name());
            if (levels)
            {
                listDir(file.name(), levels - 1);
            }
        }
        else
        {
            LOG_Println("  FILE: ");
            LOG_Println(file.name());
            LOG_Println("  SIZE: ");
            LOG_Println(file.size());
        }
        file = root.openNextFile();
    }
}

String FSHandler::jsonifyDir(String dir, String ext)
{
    String jsonString;

    if (_state != NO_INIT)
    {
        File root = _FSRoot->open(dir);
        if (!root || !root.isDirectory())
        {
            return "";
        }

        File file = root.openNextFile();
        jsonString = "{\"files\":[";

        while (file)
        {
            if (!file.isDirectory())
            {
                String fileName = file.name();

                if (fileName.endsWith(ext))
                {
                    jsonString += "{\"filename\":\"" + fileName + "\"," + // JSON requires double quotes
                                  "\"size\":\"" + (String)file.size() + "\"},";
                }
            }
            file = root.openNextFile();
        }
        jsonString[jsonString.length() - 1] = ']';
        jsonString += "}";
        // jsonString += "}";

        LOG_Println(jsonString);

        return jsonString;
    }
    else
        return "";
}

void FSHandler::writeBytes(String &path, const uint8_t *data, size_t len)
{
    _state = IDLE;

    File file = _FSRoot->open(path, FILE_WRITE);
    if (!file)
    {
        LOG_Println("Failed to open file for writing");
        _state = ERROR;
        return;
    }
    _state = WRITING;
    if (file.write(data, len) == len)
    {
        _state = SUCCESS;
    }
    else
    {
        LOG_Println("Write failed");
        _state = ERROR;
    }
    file.close();
}
