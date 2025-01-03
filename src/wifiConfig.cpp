#include <Esp.h>
#include <TimeLib.h>
#include "wifiConfig.hpp"
#include "appStati.hpp"

namespace measure_h2o
{

  const char *WifiConfig::tag{ "WifiConfig" };
  WiFiManager WifiConfig::wm;

  /**
   * initialize the static object
   */
  void WifiConfig::init()
  {
    char hostname[ 32 ];
    uint16_t chip = static_cast< uint16_t >( ESP.getEfuseMac() >> 32 );
    snprintf( hostname, 32, "%s-%08X", prefs::DEFAULT_HOSTNAME, chip );
    WiFi.setHostname( hostname );
    WiFi.mode( WIFI_STA );
    WiFi.onEvent( WifiConfig::wifiEventCallback );
    WifiConfig::wm.setConfigPortalBlocking( true );
    WifiConfig::wm.setConnectTimeout( 10 );
#ifdef BUILD_DEBUG
    WifiConfig::wm.setConfigPortalTimeout( 90 );  // 90s minutes up to auto connect again
#else
    WifiConfig::wm.setConfigPortalTimeout( 180 );  // 2 minutes up to auto connect again
#endif
    WifiConfig::wm.setConnectRetries( 5 );               // retry 5 times to reconnect
    WifiConfig::wm.setAPCallback( configModeCallback );  // callback when manager starts
    //
    // set an callback for my reasons
    //
    sntp_set_time_sync_notification_cb( WifiConfig::timeSyncNotificationCallback );
    WifiConfig::reInit();
  }

  void WifiConfig::reInit()
  {
    elog.log( INFO, "%s: initialize wifi...", WifiConfig::tag );
    prefs::AppStati::setWlanState( WlanState::SEARCHING );
    String msg = "verbinde WiFi...";
    display->printLine( msg );
    if ( !WifiConfig::wm.autoConnect( prefs::WIFI_CONFIG_AP, prefs::WIFI_CONFIG_PASS ) )
    {
      prefs::AppStati::setWlanState( WlanState::FAILED );
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
    WifiConfig::timeResync();
    WifiConfig::wm.stopWebPortal();
    elog.log( INFO, "%s: initialize wifi...OK", WifiConfig::tag );
  }

  /**
   * sync or resync time if wifi connected
   */
  void WifiConfig::timeResync()
  {
    if ( prefs::AppStati::getWlanState() == WlanState::CONNECTED || prefs::AppStati::getWlanState() == WlanState::TIMESYNCED )
    {
      elog.log( INFO, "%s: wifi connected, try to (re)sync time...", WifiConfig::tag );
      configTime( prefs::AppStati::getTimezoneOffset(), 0, prefs::NTP_POOL_00, prefs::NTP_POOL_01 );
    }
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
        if ( prefs::AppStati::getWlanState() != WlanState::SEARCHING )
        {
          WifiConfig::wm.disconnect();
          // WiFi.mode( WIFI_OFF );
          delay( 500 );
          WifiConfig::reInit();
        }
        break;
      case SYSTEM_EVENT_STA_GOT_IP:
        elog.log( INFO, "%s: device got ip <%s>...", WifiConfig::tag, WiFi.localIP().toString().c_str() );
        if ( prefs::AppStati::getWlanState() == WlanState::DISCONNECTED )
          prefs::AppStati::setWlanState( WlanState::CONNECTED );
        WifiConfig::timeResync();
        break;
      case SYSTEM_EVENT_STA_LOST_IP:
        elog.log( INFO, "%s: device lost ip...", WifiConfig::tag );
        prefs::AppStati::setWlanState( WlanState::DISCONNECTED );
        WifiConfig::reInit();
        break;
      case SYSTEM_EVENT_AP_STACONNECTED:
        elog.log( INFO, "%s: WIFI client connected...", WifiConfig::tag );
        msg = "WiFi client conn";
        display->printLine( msg );
        break;
      case SYSTEM_EVENT_AP_STADISCONNECTED:
        elog.log( INFO, "%s: WIFI client disconnected...", WifiConfig::tag );
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
        }
    }
  }

  /**
   * if an config event from wifi manager occurs, tell me that
   */
  void WifiConfig::configModeCallback( WiFiManager *myWiFiManager )
  {
    elog.log( INFO, "%s: enter WiFi config mode...", WifiConfig::tag );
    prefs::AppStati::setWlanState( WlanState::CONFIGPORTAL );
    IPAddress apAddr = WiFi.softAPIP();
    elog.log( INFO, "%s: Access Point IP: <%s>...", WifiConfig::tag, apAddr.toString() );
    auto pSSID = myWiFiManager->getConfigPortalSSID();
    String msg = String( "IP: " ) + apAddr.toString();
    display->printLine( msg );
    display->printLine( pSSID );
    elog.log( INFO, "%s: AP SSID: <%s>...", WifiConfig::tag, pSSID.c_str() );
  }

}  // namespace measure_h2o
