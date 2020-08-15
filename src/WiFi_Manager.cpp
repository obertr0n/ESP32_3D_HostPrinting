#include <functional>

#include "WiFi_Manager.h"
#include "Config.h"
#include "Log.h"
#include "SPIFFS.h"

using namespace std;

void WiFiManagerClass::begin()
{
    SPIFFS.begin();
    _pref.begin(PREF_WIFI_NAMESPACE.c_str());

    _ssid = getStringPref(PREF_KEY_WIFI_SSID);
    _pass = getStringPref(PREF_KEY_WIFI_PASS);
    _wifiMode = (WiFiMode_t)getWifiModePref(PREF_KEY_WIFI_MODE);

    /* check how we should start based on Preference */
    if(WIFI_AP == _wifiMode)
    {
        startAP();
    }
    else if(WIFI_STA == _wifiMode)
    {
        /* check if stored details are working OK */
        if(!startSTA())
        {
            LOG_Println("Captive started");
            startCaptive();
            loopCaptive();
        }
    }
}

void WiFiManagerClass::startCaptive()
{
    /* if we are here, we either failed WiFi connection */
    /* start in AP mode */
    startAP();
    _dns->setErrorReplyCode(DNSReplyCode::NoError);
    /* any request will be captured */
    _dns->start(DNS_PORT, "*", WiFi.softAPIP());

    _server->onNotFound(bind(&WiFiManagerClass::webServerHandleNotFound, this, placeholders::_1));
    _server->on("/", HTTP_GET, bind(&WiFiManagerClass::webServerGETRoot, this, placeholders::_1));
    _server->on("/www/captive.css", HTTP_GET, bind(&WiFiManagerClass::webServerGETLoadCSS, this, placeholders::_1));
    _server->on("/reqwifi", HTTP_ANY, bind(&WiFiManagerClass::webServerANYWifReq, this, placeholders::_1));
    _server->begin();
}

void WiFiManagerClass::webServerHandleNotFound(AsyncWebServerRequest *request)
{
    LOG_Println("In not found");
    request->redirect("/");
}

void WiFiManagerClass::webServerGETLoadCSS(AsyncWebServerRequest *request)
{
    LOG_Println("Serve css");
    request->send(SPIFFS, "/www/captive.css", "text/css");
}

void WiFiManagerClass::webServerGETRoot(AsyncWebServerRequest *request)
{
    LOG_Println("Serve root");
    request->send(SPIFFS, "/www/captive.html", "text/html");
}

void WiFiManagerClass::webServerANYWifReq(AsyncWebServerRequest *request)
{
    LOG_Println("Serve req");
    if (request->hasArg("refresh"))
    {
        String result = "{\"networks\":[";
        int16_t numNetworks = WiFi.scanNetworks();
        for (int16_t netId = 0; netId < numNetworks; netId++)
        {
            result += "{\"ssid\":\"" + WiFi.SSID(netId) + "\",";
            result += "\"rssi\":\"" + static_cast<String>(WiFi.RSSI(netId)) + "\",";
            result += "\"chan\":\"" + static_cast<String>(WiFi.channel(netId)) + "\",";
            result += "\"type\":\"" + static_cast<String>(WiFi.encryptionType(netId)) + "\",";
            result += "\"bssid\":\"" + WiFi.BSSIDstr(netId) + "\"},";
        }
        result[result.length() - 1] = ']';
        result += "}";

        LOG_Println(result);
    }
    else if(request->hasArg("mode"))
    {
        String mode = request->arg("mode");
        if(mode = "ap")
        {
            _wifiMode = WIFI_AP;
        }
        else if(mode = "sta")
        {
            _wifiMode = WIFI_STA;
        }
        setWifiModePref(PREF_KEY_WIFI_MODE, (uint8_t)_wifiMode);
    }
    else if(request->hasArg("connect"))
    {

    }
}

void WiFiManagerClass::startAP()
{
    LOG_Println("Starting in AP mode");
    WiFi.softAP(AP_DEFAULT_SSID, AP_DEFAULT_PASS);
    WiFi.softAPConfig(_softApIP, _softApIP, _softApSnet);
    WiFi.begin();
}

bool WiFiManagerClass::startSTA()
{
    const char* pass = NULL;

    if (_pass != "")
    {
        pass = _pass.c_str();
    }
    else if (_ssid != "")
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid.c_str(), pass);

        uint32_t timeout = millis() + CONNECTION_TIMEOUT;

        while ((WiFi.status() != WL_CONNECTED) &&
            (timeout > millis()))
        {
            LOG_Println(".");
            delay(500);
        }
        /* connection failed or timmedout */
        if(WiFi.status() == WL_DISCONNECTED)
        {
            return false;
        }
        else if(WiFi.status() == WL_CONNECTED)
        {
            return true;
        }
    }
    return false;
}

void WiFiManagerClass::setStringPref(String const &key, String const &val)
{
    _pref.putString(key.c_str(), val);
}

String WiFiManagerClass::getStringPref(String const &key)
{
    return _pref.getString(key.c_str(), "");
}

void WiFiManagerClass::setWifiModePref(String const &key, uint8_t val)
{
    _pref.putUChar(key.c_str(), val);
}

uint8_t WiFiManagerClass::getWifiModePref(String const &key)
{
    return _pref.getUChar(key.c_str(), WIFI_STA);
}

void WiFiManagerClass::loopCaptive()
{
    for(;;)
    {
        _dns->processNextRequest();
    }
}

WiFiManagerClass WiFiManager;