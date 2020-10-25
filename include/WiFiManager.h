#ifndef WIFI_MANAGER_h
#define WIFI_MANAGER_h

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <inttypes.h>

class WiFiManagerClass
{
    public:
        WiFiManagerClass()
        {
            _softApIP = IPAddress(192, 168, 0, 150);
            _softApSnet = IPAddress(255,255,255,0);

            _needConfig = false;
            _doReset = false;
            _ssid = "";
            _pass = "";
        };
        ~WiFiManagerClass()
        {   
            if(_server)
            {
                _server->end();
                delete _server;
            }
            if(_dns)
            {
                _dns->stop();
                delete _dns;
            }
        };

        void resetSetting();
        void begin();
        void loop();
    private:
        const String PREF_WIFI_NAMESPACE    = "ns_wifi";
        const String PREF_KEY_WIFI_SSID     = "wifi_ssid";
        const String PREF_KEY_WIFI_PASS     = "wifi_pass";
        const String PREF_KEY_WIFI_MODE     = "wifi_mode";

        const uint16_t DNS_PORT             = 53;
        /* timeout for connecting to a network */
        const uint32_t CONNECTION_TIMEOUT   = 10*1000;


        Preferences _pref;
        AsyncWebServer* _server;
        DNSServer* _dns;

        IPAddress _softApIP;
        IPAddress _softApSnet;

        String _ssid;
        String _pass;
        WiFiMode_t _wifiMode;
        bool _needConfig;
        bool _doReset;

        void beginCaptive();
        bool startSTA();
        void startAP();
        void loopCaptive();

        void webServerHandleNotFound(AsyncWebServerRequest *request);
        void webServerGETLoadCSS(AsyncWebServerRequest *request);
        void webServerGETRoot(AsyncWebServerRequest *request);
        void webServerANYWifReq(AsyncWebServerRequest *request);
        
        String getStringPref(String const& key);
        void setStringPref(String const& key, String const& val);
        uint8_t getWifiModePref(String const &key);
        void setWifiModePref(String const &key, uint8_t val);

};

extern WiFiManagerClass wifiManager;


#endif /* WIFI_MANAGER_h */