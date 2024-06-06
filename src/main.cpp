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
#include "statics.hpp"
#include "wifiConfig.hpp"
#include "webServer.hpp"
#include "main.hpp"

constexpr uint64_t DELAYTIME = 750000ULL;
constexpr uint64_t HARTBEATTIME = 400000ULL;
constexpr uint64_t ANTTIME = 1200000ULL;
constexpr uint64_t CALIBRTIME = 255000ULL;
constexpr uint64_t FORCE_DELAYTIME = 10000000ULL;

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
  // not correct functional on this chip :-(
  //
  // elog.log( DEBUG, "main: set timezone (%s)...", prefs::AppStati::getTimeZone().c_str() );
  // setenv( "TZ", prefs::AppStati::getTimeZone().c_str(), 1 );
  // tzset();
  //
  // led stripe settings
  //
  elog.log( INFO, "main: init LEDSTRIPE..." );
  mLED = std::make_shared< MLED >( prefs::LED_NUM, prefs::LED_INTERNAL, NEO_GRB + NEO_KHZ800 );
  mLED->setBrightness( 128 );  // Set BRIGHTNESS  (max = 255)
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

  static uint64_t setNextTimeCorrect{ ( 1000ULL * 1000ULL * 21600ULL ) };
  static uint64_t nextTimeToDisplayValues = DELAYTIME;
  static uint64_t nextTimeHartbeat = HARTBEATTIME;
  static uint64_t nextAntTime = ANTTIME;
  static uint64_t nextTimeCalibrCheck = CALIBRTIME;
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
    auto result = showColors();
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
 * zyclic called function to set time from NTP
 */
time_t getNtpTime()
{
  struct tm ti;
  if ( getLocalTime( &ti ) )
  {
    setTime( ti.tm_hour, ti.tm_min, ti.tm_sec, ti.tm_mday, ti.tm_mon + 1, ti.tm_year + 1900 );
    return now();
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
      if ( prefs::CURRENT_BORDER_FOR_CALIBR < prefs::AppStati::getCurrentMiliVolts() )
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
 * show pressure values
 */
int showColors()
{
  using namespace measure_h2o;

  //
  // DEMONSTRATION ONLY, IMPLEMENT LATER
  //
  uint8_t red = static_cast< uint8_t >( 0xff & random( 255 ) );
  uint8_t green = static_cast< uint8_t >( 0xff & random( 255 ) );
  uint8_t blue = static_cast< uint8_t >( 0xff & random( 255 ) );
  mLED->setBrightness( static_cast< uint8_t >( 0xff & random( 0x20, 0xff ) ) );
  mLED->setPixelColor( 0, mLED->Color( red, green, blue ) );
  mLED->show();
  delay( 150 );
  mLED->setPixelColor( 0, 0x0 );
  mLED->show();
  return 0;
}

void checkOnlineState()
{
  using namespace measure_h2o;
  static WlanState currWLANState{ WlanState::DISCONNECTED };
  String msg;

  if ( currWLANState != prefs::AppStati::getWlanState() )
  {
    auto new_connected = prefs::AppStati::getWlanState();
    if ( currWLANState == WlanState::DISCONNECTED || currWLANState == WlanState::FAILED || currWLANState == WlanState::SEARCHING )
    {
      //
      // was not functional for webservice
      //
      if ( new_connected == WlanState::CONNECTED || new_connected == WlanState::TIMESYNCED )
      {
        if ( new_connected == WlanState::CONNECTED )
        {
          msg = "WiFi verbunden!";
          display->printLine( msg );
          msg = "Warte auf TIME";
          display->printLine( msg );
        }
        else
        {
          msg = "Zeit sync OK!";
          display->printLine( msg );
        }
        //
        // new connection, start webservice
        //
        elog.log( INFO, "main: ip connectivity found, start webserver." );
        APIWebServer::start();
      }
      else
      {
        msg = "WiFi getrennt!";
        display->printLine( msg );
        elog.log( WARNING, "main: ip connectivity lost, stop webserver." );
        APIWebServer::stop();
      }
    }
    else
    {
      //
      // was functional for webservice
      //
      if ( !( new_connected == WlanState::CONNECTED || new_connected == WlanState::TIMESYNCED ) )
      {
        //
        // not longer functional
        //
        msg = "WiFi getrennt!";
        display->printLine( msg );
        elog.log( WARNING, "main: ip connectivity lost, stop webserver." );
        APIWebServer::stop();
      }
    }
    // mark new value
    currWLANState = new_connected;
  }
}

void updateDisplay()
{
  using namespace measure_h2o;
  static uint64_t nextTimeToForceShowPresure = FORCE_DELAYTIME;
  //
  // set display if changed
  //
  if ( prefs::AppStati::getWasChanged() || esp_timer_get_time() > nextTimeToForceShowPresure )
  {
    uint32_t mVolt = prefs::AppStati::getCurrentMiliVolts();
    float volt = mVolt / 1000.0f;
    float pressureBar = prefs::AppStati::getCurrentPressureBar();
    if ( esp_timer_get_time() <= nextTimeToForceShowPresure )
      elog.log( INFO, "main: pressure changed, raw value <%1.2f V>, pressure <%1.2f bar>", volt, pressureBar );
    //
    display->printTension( volt );
    display->printPresure( pressureBar );
    prefs::AppStati::resetWasChanged();
    nextTimeToForceShowPresure = esp_timer_get_time() + FORCE_DELAYTIME;
  }
}