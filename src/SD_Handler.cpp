#include <SD.h>
#include <SPI.h>

#include "SD_Handler.h"
#include "HP_Config.h"

bool SdHandler::begin()
{
    SPI.begin(14, 2, 15, 13);

    if (SD.begin(13))
    {
        LOG_Println("SD Card initialized.");

        _SDRoot = &SD;
        _state = IDLE;

        return true;
    }
    return false;
}

void SdHandler::listDir(const char* dirname, uint8_t levels)
{
    File root = _SDRoot->open(dirname);
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

String SdHandler::jsonifyDir(String dir, String ext)
{
    String jsonString;

    if (_state != NO_INIT)
    {
        File root = _SDRoot->open(dir);
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
        jsonString[jsonString.length()-1] = ']';
        jsonString += "}";
        // jsonString += "}";

        LOG_Println(jsonString);
        
        return jsonString;
    }
    else
        return "";
}

void SdHandler::writeBytes(String& path, const uint8_t* data, size_t len)
{
    _state = IDLE;

    File file = _SDRoot->open(path, FILE_WRITE);
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
