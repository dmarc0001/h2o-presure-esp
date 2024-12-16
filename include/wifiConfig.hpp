#pragma once
#include "appPrefs.hpp"
#include "statics.hpp"
#include <esp_sntp.h>
#include <WiFiManager.h>

namespace measure_h2o
{
  class WifiConfig
  {
    private:
    static const char *tag;                    //! for debugging
    static bool is_sntp_init;                  //! was sntp
    //static WiFiManagerParameter custom_field;  //! for non-blocking

    public:
    static WiFiManager wm;  //! global wm instance

    public:
    static void init();    //! init wifi and/or wifi manager
    static void reInit();  //! reinit wifi and/or wifi manager
    static void loop();    //! loop for webserver wifi callmanager

    private:
    static void timeSyncNotificationCallback( struct timeval * );  // callback for ntp
    static void wifiEventCallback( arduino_event_t * );            //! callback wifi events
    static void configModeCallback( WiFiManager *myWiFiManager );  //! callback for wifi manager ebent
  };
}  // namespace measure_h2o
