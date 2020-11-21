#include "FileSysHandler.h"
#include "Log.h"

FSHandler fileHandler;

bool FSHandler::begin()
{
    #if (ON == ENABLE_SD_CARD)
    /* init SPI for SD communication */
    SPI.begin(PIN_SD_CLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_SS);
    #endif
    /* at the moment we don't know the FS type */
    _storageType = FS_NONE;
    _FSRoot = NULL;
    _state = ERROR;
    bool ret = false;
    if (SPIFFS.begin())
    {
        hp_log_printf("SPIFFS mounted.\n");
        _storageType = FS_SPIFFS;
        _FSRoot = &SPIFFS;
        _state = IDLE;
        _pathMaxLen = 11;
        ret = true;
    }
    /* try and mount SD */
    #if (ON == ENABLE_SD_CARD)
    if (SD.begin(PIN_SD_SS))
    {
        hp_log_printf("SD Card initialized.\n");
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
    #endif
    return ret;
}

void FSHandler::listDir(const char *dirname, uint8_t levels)
{
    File root = _FSRoot->open(dirname);
    if (!root)
    {
        hp_log_printf("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        hp_log_printf("Not a directory\n");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            hp_log_printf("  DIR : %s\n", file.name());
            if (levels)
            {
                listDir(file.name(), levels - 1);
            }
        }
        else
        {
            hp_log_printf("  FILE: %s\n", file.name());
            hp_log_printf("  SIZE: %d\n", file.size());
        }
        file = root.openNextFile();
    }
}

String FSHandler::jsonifyDir(String dir, String ext)
{
    String jsonString = "{\"files\":[]}";
    bool found = false;

    if (_state != NOT_INIT)
    {
        File root = _FSRoot->open(dir);
        if (!root || !root.isDirectory())
        {
            hp_log_printf("invalid dir\n");
            return jsonString;
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
                    found = true;
                }
            }
            file = root.openNextFile();
        }
        if(found)
        {
            jsonString[jsonString.length() - 1] = ']';
            jsonString += "}";
        }
        else
        {
            jsonString = "{\"files\":[]}";
        }
        
        hp_log_printf("%s\n", jsonString.c_str());

        return jsonString;
    }
    return jsonString;
}

void FSHandler::writeBytes(String &path, const uint8_t *data, size_t len)
{
    _state = IDLE;

    File file = _FSRoot->open(path, FILE_WRITE);
    if (!file)
    {
        hp_log_printf("Failed to open file for writing\n");
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
        hp_log_printf("Write failed\n");
        _state = ERROR;
    }
    file.close();
}
