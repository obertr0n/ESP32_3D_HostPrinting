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
            _server = new AsyncWebServer(80);
            _dns = new DNSServer();

            _softApIP = IPAddress(8, 8, 8, 8);
            _softApSnet = IPAddress(255,255,255,0);

            _needConfig = false;
            _ssid = "";
            _pass = "";
        };
        ~WiFiManagerClass()
        {   
            _server->end();
            delete _server;

            _dns->stop();
            delete _dns;
        };

        void reset();
        void restart();
        void begin();
        void loop();
    private:
        const String PREF_WIFI_NAMESPACE    = "ns_wifi";
        const String PREF_KEY_WIFI_SSID     = "wifi_ssid";
        const String PREF_KEY_WIFI_PASS     = "wifi_pass";
        const String PREF_KEY_WIFI_MODE     = "wifi_mode";

        const uint16_t DNS_PORT             = 53;
        /* timeout for connecting to a network */
        const uint32_t CONNECTION_TIMEOUT   = 20*1000;


        Preferences _pref;
        AsyncWebServer* _server;
        DNSServer* _dns;

        IPAddress _softApIP;
        IPAddress _softApSnet;

        String _ssid;
        String _pass;
        WiFiMode_t _wifiMode;
        bool _needConfig;

        void startCaptive();
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

extern WiFiManagerClass WiFiManager;


#endif /* WIFI_MANAGER_h */