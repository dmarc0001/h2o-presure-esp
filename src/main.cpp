
#include <Esp.h>
#include <stdio.h>
#include <Arduino.h>
#include <memory>
#include <cstdio>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <Elog.h>
#include <TimeLib.h>
#include "filesystem.hpp"
#include "appStati.hpp"
#include "appPrefs.hpp"
#include "fileService.hpp"
#include "ledStripe.hpp"
#include "statics.hpp"
#include "wifiConfig.hpp"
#include "webServer.hpp"
#include "main.hpp"

constexpr int64_t DELAYTIME = 750000LL;
constexpr int64_t HARTBEATTIME = 400000LL;
constexpr int64_t ANTTIME = 1200000LL;
constexpr int64_t CALIBRTIME = 255000LL;
constexpr int64_t FORCE_DELAYTIME = 4000000LL;

void setup()
{
  using namespace measure_h2o;

  // Debug Ausgabe init
  Serial.begin( 115200 );
  Serial.println( "main: program started..." );
  Filesystem::init();
  // sleep( 5 );
  Loglevel level = DEBUG;
  prefs::AppStati::init();
  level = static_cast< Loglevel >( prefs::AppStati::getLogLevel() );
  elog.addSerialLogging( Serial, "GLO", level );
  elog.log( INFO, "main: start with logging..." );
  //
  // file storage init
  //
  FileService::init();
  //
  // timezone settings
  // not correct functional on this c3 chip :-(
  //
  // elog.log( DEBUG, "main: set timezone (%s)...", prefs::AppStati::getTimeZone().c_str() );
  // setenv( "TZ", prefs::AppStati::getTimeZone().c_str(), 1 );
  // tzset();
  // start analog reader
  elog.log( INFO, "main: init preasure measure..." );
  PrSensor::init();
  // display init
  elog.log( INFO, "main: waveshare display init..." );
  display = std::make_shared< MLCD >( prefs::DISPLAY_COLS, prefs::DISPLAY_ROWS, prefs::DISPLAY_SDA_PIN, prefs::DOSPLAY_SCL_PIN );
  display->init();
  //
  // watch for calibrating request
  //
  pinMode( static_cast< uint8_t >( prefs::CALIBR_REQ_PIN ), INPUT_PULLUP );
  //
  // led stripe settings
  //
  elog.log( INFO, "main: init LEDSTRIPE..." );
  measure_h2o::MLED::init( prefs::LED_NUM, prefs::LED_PIN, NEO_GRB + NEO_KHZ800 );
  //
  // WLAN/Networking init
  //
  static String hName( prefs::AppStati::getHostName() );
  elog.log( INFO, "main: hostname: <%s>...", hName.c_str() );
  elog.log( DEBUG, "main: start wifi..." );
  String msg( "start APP..." );
  sleep( 3 );
  display->printLine( msg );
  msg = "init WIFI...";
  display->printLine( msg );
  WifiConfig::init();
  sleep( 2 );
  display->printGreeting();
  //
  //  random init
  //
  randomSeed( analogRead( prefs::RAND_PIN ) );
  //
  // timeLib sync Time width system
  //
  setSyncInterval( 240 );
  setSyncProvider( getNtpTime );
  sleep( 3 );
  display->clear();
}

void loop()
{
  using namespace measure_h2o;

  static int64_t setNextTimeCorrect{ ( 1000LL * 1000LL * 21600LL ) };
  static int64_t nextTimeToDisplayValues = DELAYTIME;
  static int64_t nextTimeHartbeat = HARTBEATTIME;
  static int64_t nextAntTime = ANTTIME;
  static int64_t nextTimeCalibrCheck = CALIBRTIME;
  static int64_t nextTimePanicReboot{ 0 };
  static bool antMarkShow{ false };
  uint64_t nowTime = esp_timer_get_time();

  if ( setNextTimeCorrect < nowTime )
  {
    //
    // somtimes correct elog time
    //
    setNextTimeCorrect = nowTime + ( 1000ULL * 1000ULL * 21600ULL );
    correctTime();
  }

  if ( nowTime > nextTimeCalibrCheck )
  {
    //
    // time to check if the master whish to calibre
    //
    nextTimeCalibrCheck = nowTime + CALIBRTIME;
    auto result = controlCalibr();
    if ( result > 0 )
      nextTimeCalibrCheck = nowTime + 10000000ULL;
  }

  if ( nowTime > nextTimeToDisplayValues )
  {
    //
    // lets actualize the preasure display
    //
    nextTimeToDisplayValues = nowTime + DELAYTIME;
    checkOnlineState();
    updateDisplay();
  }
  //
  // hartbeat
  //
  if ( nowTime > nextTimeHartbeat )
  {
    //
    // lets show the heartbeat
    //
    nextTimeHartbeat = nowTime + HARTBEATTIME;
    //
    // hartbeat
    //
    display->printHartbeat();
  }
  //
  // antenna symbol
  //
  if ( nowTime > nextAntTime )
  {
    //
    // show ant if WiFi
    //
    nextAntTime = nowTime + ANTTIME;
    if ( prefs::AppStati::getWlanState() == prefs::WlanState::TIMESYNCED )
    {
      if ( antMarkShow )
      {
        display->printAntMark();
        nextAntTime = nowTime + ( ANTTIME << 1 );
      }
      else
      {
        display->hideAntMark();
      }
      antMarkShow = !antMarkShow;
    }
    else
    {
      display->hideAntMark();
    }
  }
}

/**
 * cyclic called function to set time from NTP
 */
time_t getNtpTime()
{
  if ( prefs::AppStati::getWlanState() == prefs::WlanState::TIMESYNCED )
  {
    struct tm ti;
    if ( getLocalTime( &ti ) )
    {
      setTime( ti.tm_hour, ti.tm_min, ti.tm_sec, ti.tm_mday, ti.tm_mon + 1, ti.tm_year + 1900 );
      return now();
    }
  }
  return 0;
}

/**
 * correct time for logger
 */
void correctTime()
{
  using namespace measure_h2o;
  //
  // somtimes correct elog time
  //
  elog.log( DEBUG, "main: logger time correction..." );
  if ( prefs::AppStati::getWlanState() == prefs::WlanState::TIMESYNCED )
  {
    struct tm ti;
    if ( !getLocalTime( &ti ) )
    {
      elog.log( WARNING, "main: failed to obtain system time!" );
    }
    else
    {
      elog.log( DEBUG, "main: gotten system time!" );
      Elog::provideTime( ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday, ti.tm_hour, ti.tm_min, ti.tm_sec );
      elog.log( DEBUG, "main: logger time correction...OK" );
    }
  }
  else
  {
    elog.log( WARNING, "main: no time sync, no correction!" );
  }
}

/**
 * control calibre key
 */
int controlCalibr()
{
  using namespace measure_h2o;

  auto cl_switch = digitalRead( prefs::CALIBR_REQ_PIN );
  if ( cl_switch == LOW )
  {
    //
    // there was an low impulse, is this permanent
    //
    elog.log( DEBUG, "main: calibrating requested???" );
    //
    // wait for mechanic switch
    //
    delay( 20 );
    //
    // after that, always low?
    //
    cl_switch = digitalRead( prefs::CALIBR_REQ_PIN );
    if ( cl_switch == LOW )
    {
      //
      // TODO: wait for key UP
      // TODO: long keydown RESET device
      //
      String msg( "Druck erkannt!" );
      String nix( "nicht moeglich" );
      // maybe i can calibr?
      elog.log( DEBUG, "main: calibrating requested!" );
      // is ther a preasure in the sensor?
      if ( prefs::CURRENT_BORDER_FOR_CALIBR < PrSensor::getCurrentValue() )
      {
        display->printAlert( msg );
        delay( 2000 );
        while ( digitalRead( prefs::CALIBR_REQ_PIN ) == LOW )
        {
          display->printAlert( nix );
          delay( 800 );
          display->printAlert( msg );
          delay( 1000 );
        }
        return 1;
      }
      else
      {
        //
        // i should calibr the device
        //
        elog.log( DEBUG, "main: call calibre routine..." );
        String msg( "calibriere..." );
        display->printMessage( msg );
        //
        // calibrating sensor
        //
        PrSensor::calibreSensor();
        msg = "fertig...       ";
        display->printMessage( msg );
        elog.log( DEBUG, "main: call calibre routine...done" );
        elog.log( INFO, "calibre routine done" );
        while ( digitalRead( prefs::CALIBR_REQ_PIN ) == LOW )
        {
          delay( 100 );
          taskYIELD();
        }
        elog.log( DEBUG, "main: continue" );
      }
    }
  }
  return 0;
}

/**
 * check if the system is online
 */
void checkOnlineState()
{
  using namespace measure_h2o;
  static WlanState oldWLANState{ WlanState::DISCONNECTED };
  static int64_t timeSyncTimeOut{ prefs::NTP_SYNC_TIMEOUT_YS << 1 };
  // int64_t nowTime = esp_timer_get_time();
  String msg;

  //
  // if an timeout while timeSync
  //
  if ( timeSyncTimeOut > 0LL )
  {
    if ( timeSyncTimeOut < esp_timer_get_time() )
    {
      String msg = "Timesync Err!";
      display->printAlert( msg );
      elog.log( CRITICAL, "main: no time sync restart Controller!" );
      delay(5000);
      ESP.restart();
    }
  }
  //
  // has online state changed?
  //
  if ( oldWLANState != prefs::AppStati::getWlanState() )
  {
    auto newWLANState = prefs::AppStati::getWlanState();
    //
    // yes state changed
    //
    if ( oldWLANState == WlanState::DISCONNECTED || oldWLANState == WlanState::FAILED || oldWLANState == WlanState::SEARCHING )
    {
      //
      // system was not connected before
      // was not functional for webservice before
      //
      if ( newWLANState == WlanState::CONNECTED || newWLANState == WlanState::TIMESYNCED )
      {
        // system is now connected
        if ( newWLANState == WlanState::CONNECTED )
        {
          // connected but not time synced
          msg = "WiFi verbunden!";
          display->printLine( msg );
          msg = "Warte auf TIME";
          display->printLine( msg );
          // 20 sec timeout für timesync
          timeSyncTimeOut = esp_timer_get_time() + prefs::NTP_SYNC_TIMEOUT_YS;
        }
        else
        {
          // connected AND time synced
          msg = "Zeit sync OK!";
          display->printLine( msg );
          timeSyncTimeOut = 0LL;
        }
        //
        // new connection, start webservice
        //
        elog.log( INFO, "main: ip connectivity found, start webserver." );
        delay( 500 );
        APIWebServer::start();
      }
      else
      {
        //
        // system is now disconnected
        //
        msg = "WiFi getrennt!";
        display->printLine( msg );
        elog.log( WARNING, "main: ip connectivity lost!" );
        timeSyncTimeOut = 0LL;
      }
    }
    else
    {
      //
      // was functional for webservice before
      //
      if ( !( newWLANState == WlanState::CONNECTED || newWLANState == WlanState::TIMESYNCED ) )
      {
        //
        // not longer functional
        //
        msg = "WiFi getrennt!";
        display->printLine( msg );
      }
    }
    // mark new value
    oldWLANState = newWLANState;
  }
}

/**
 * update the display
 */
void updateDisplay()
{
  using namespace measure_h2o;
  static uint64_t nextTimeToForceShowPresure = FORCE_DELAYTIME;
  static int hour, minute, count;
  //
  // set display if changed
  //
  if ( prefs::AppStati::getWasChanged() || esp_timer_get_time() > nextTimeToForceShowPresure )
  {
    float pressureBar = prefs::AppStati::getCurrentPressureBar();
#ifdef BUILD_DEBUG
    uint32_t mVolt = prefs::AppStati::getCurrentMiliVolts();
    float volt = mVolt / 1000.0f;
    if ( prefs::AppStati::getWasChanged() )
      elog.log( INFO, "main: pressure changed, raw value <%1.2f V>, pressure <%1.2f bar>", volt, pressureBar );
#else
    if ( prefs::AppStati::getWasChanged() )
      elog.log( INFO, "main: pressure changed, pressure <%1.2f bar>", pressureBar );
#endif
    if ( esp_timer_get_time() > nextTimeToForceShowPresure )
    {
      sntp_sync_status_t tsyncStatus = sntp_get_sync_status();
      if ( prefs::AppStati::getWlanState() == WlanState::TIMESYNCED || SNTP_SYNC_STATUS_IN_PROGRESS == tsyncStatus )
      {
        struct tm ti;
        if ( getLocalTime( &ti ) )
        {
          if ( hour != ti.tm_hour || minute != ti.tm_min || count > 4 )
          {
            // only if time changed
            hour = ti.tm_hour;
            minute = ti.tm_min;
            count = 0;
            char buffer[ 12 ];
            if ( SNTP_SYNC_STATUS_IN_PROGRESS == tsyncStatus )
              snprintf( buffer, 8, "*%02d:%02d*", ti.tm_hour, ti.tm_min );
            else
              snprintf( buffer, 6, "%02d:%02d", ti.tm_hour, ti.tm_min );
            String timeStr( buffer );
            display->printTime( timeStr );
          }
          ++count;
        }
      }
      else
      {
        if ( SNTP_SYNC_STATUS_COMPLETED == tsyncStatus )
          prefs::AppStati::setWlanState( WlanState::TIMESYNCED );
        display->printTime( "--:--" );
      }
    }
    display->printPresure( pressureBar );
    prefs::AppStati::resetWasChanged();
    nextTimeToForceShowPresure = esp_timer_get_time() + FORCE_DELAYTIME;
  }
}