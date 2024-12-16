#include <Esp.h>
#include <TimeLib.h>
#include "wifiConfig.hpp"
#include "appStati.hpp"

namespace measure_h2o
{

  const char *WifiConfig::tag{ "WifiConfig" };
  bool WifiConfig::is_sntp_init{ false };
  WiFiManager WifiConfig::wm;
  // WiFiManagerParameter WifiConfig::custom_field;

  /**
   * initialize the static object
   */
  void WifiConfig::init()
  {
    char hostname[ 32 ];
    // elog.log( INFO, "%s: initialize wifi...", WifiConfig::tag );
    uint16_t chip = static_cast< uint16_t >( ESP.getEfuseMac() >> 32 );
    snprintf( hostname, 32, "%s-%08X", prefs::DEFAULT_HOSTNAME, chip );
    WiFi.setHostname( hostname );
    WiFi.mode( WIFI_STA );
    WiFi.onEvent( WifiConfig::wifiEventCallback );
    // reset settings - wipe credentials for testing
    // wm.resetSettings();
    // WifiConfig::wm.setConfigPortalBlocking( false );
    WifiConfig::wm.setConfigPortalBlocking( true );
    WifiConfig::wm.setConnectTimeout( 25 );
    WifiConfig::wm.setConfigPortalTimeout( 180 );        // 2 minutes up to auto connect again
    WifiConfig::wm.setConnectRetries( 5 );               // retry 5 times to reconnect
    WifiConfig::wm.setAPCallback( configModeCallback );  // callback when manager starts
    //
    // esp32 time config
    // BUG: timezone not work, using gmt offset
    configTime( prefs::AppStati::getTimezoneOffset(), 0, prefs::NTP_POOL_00, prefs::NTP_POOL_01 );
    // the old way...
    // sntp_set_sync_mode( SNTP_SYNC_MODE_IMMED );
    // sntp_setoperatingmode( SNTP_OPMODE_POLL );
    // sntp_setservername( 1, "pool.ntp.org" );

    //
    // set an callback for my reasons
    //
    sntp_set_time_sync_notification_cb( WifiConfig::timeSyncNotificationCallback );
    WifiConfig::reInit();
  }

  void WifiConfig::reInit()
  {
    elog.log( INFO, "%s: initialize wifi...", WifiConfig::tag );
    prefs::AppStati::setWlanState( WlanState::FAILED );
    String msg = "verbinde WiFi...";
    display->printLine( msg );
    if ( !WifiConfig::wm.autoConnect( prefs::WIFI_CONFIG_AP, prefs::WIFI_CONFIG_PASS ) )
    {
      String msg = "WiFi Verbindung";
      display->printAlert( msg );
      elog.log( ERROR, "%s: wifi not connected!", WifiConfig::tag );
      elog.log( ERROR, "%s: RESTART Controller", WifiConfig::tag );
      delay( 4500 );
      ESP.restart();
    }
    msg = "WiFi verbunden";
    display->printLine( msg );
    elog.log( INFO, "%s: wifi connected...", WifiConfig::tag );
    prefs::AppStati::setWlanState( WlanState::CONNECTED );
    elog.log( DEBUG, "%s: try to sync time...", WifiConfig::tag );
    sntp_init();
    WifiConfig::is_sntp_init = true;
    WifiConfig::wm.stopWebPortal();
    elog.log( INFO, "%s: initialize wifi...OK", WifiConfig::tag );
  }

  /**
   * if an wifi event occurs, tell me the event
   */
  void WifiConfig::wifiEventCallback( arduino_event_t *event )
  {
    String msg;
    switch ( event->event_id )
    {
      case SYSTEM_EVENT_STA_CONNECTED:
        elog.log( INFO, "%s: device connected to accesspoint...", WifiConfig::tag );
        break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
        elog.log( INFO, "%s: device disconnected from accesspoint...", WifiConfig::tag );
        prefs::AppStati::setWlanState( WlanState::DISCONNECTED );
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        elog.log( INFO, "%s: device got ip <%s>...", WifiConfig::tag, WiFi.localIP().toString().c_str() );
        if ( prefs::AppStati::getWlanState() == WlanState::DISCONNECTED )
          prefs::AppStati::setWlanState( WlanState::CONNECTED );
        // sntp_init();
        if ( WifiConfig::is_sntp_init )
          sntp_restart();
        else
        {
          sntp_init();
          WifiConfig::is_sntp_init = true;
        }
        break;
      case SYSTEM_EVENT_STA_LOST_IP:
        elog.log( INFO, "%s: device lost ip...", WifiConfig::tag );
        prefs::AppStati::setWlanState( WlanState::DISCONNECTED );
        sntp_stop();
        WifiConfig::is_sntp_init = false;
        break;
      case SYSTEM_EVENT_AP_STACONNECTED:
        elog.log( INFO, "%s: WIFI client connected...", WifiConfig::tag );
        msg = "WiFi client conn";
        display->printLine( msg );
        break;
      case SYSTEM_EVENT_AP_STADISCONNECTED:
        elog.log( INFO, "%s: WIFI client disconnected...", WifiConfig::tag );
        sntp_stop();
        WifiConfig::is_sntp_init = false;
        break;
      default:
        break;
    }
  }

  /**
   * if an event for systemtime, tell me this
   */
  void WifiConfig::timeSyncNotificationCallback( struct timeval * )
  {
    sntp_sync_status_t state = sntp_get_sync_status();
    switch ( state )
    {
      case SNTP_SYNC_STATUS_COMPLETED:
        elog.log( INFO, "%s: notification: time status sync completed!", WifiConfig::tag );
        ESP_LOGI( WebServer::tag, "notification: time status sync completed!" );
        if ( prefs::AppStati::getWlanState() == WlanState::CONNECTED )
        {
          prefs::AppStati::setWlanState( WlanState::TIMESYNCED );
        }
        struct tm ti;
        if ( !getLocalTime( &ti ) )
        {
          elog.log( WARNING, "%s: failed to obtain systemtime!", WifiConfig::tag );
        }
        else
        {
          elog.log( DEBUG, "%s: gotten system time!", WifiConfig::tag );
          Elog::provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
          setTime( ti.tm_hour, ti.tm_min, ti.tm_sec, ti.tm_mday, ti.tm_mon + 1, ti.tm_year + 1900 );
        }
        break;
      default:
        elog.log( INFO, "%s: notification: time status NOT sync completed!", WifiConfig::tag );
        if ( prefs::AppStati::getWlanState() == WlanState::TIMESYNCED )
        {
          prefs::AppStati::setWlanState( WlanState::CONNECTED );
          if ( WifiConfig::is_sntp_init )
            sntp_restart();
          else
          {
            sntp_init();
            WifiConfig::is_sntp_init = true;
          }
        }
    }
  }

  /**
   * if an config event from wifi manager occurs, tell me that
   */
  void WifiConfig::configModeCallback( WiFiManager *myWiFiManager )
  {
    elog.log( INFO, "%s: enter WiFi config mode...", WifiConfig::tag );
    IPAddress apAddr = WiFi.softAPIP();
    elog.log( INFO, "%s: Access Point IP: <%s>...", WifiConfig::tag, apAddr.toString() );
    auto pSSID = myWiFiManager->getConfigPortalSSID();
    String msg = String("IP: ")  + apAddr.toString();
    display->printLine(msg);
    display->printLine(pSSID);
    elog.log( INFO, "%s: AP SSID: <%s>...", WifiConfig::tag, pSSID.c_str() );
  }

}  // namespace measure_h2o
