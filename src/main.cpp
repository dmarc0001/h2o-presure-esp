#include <stdio.h>
#include <Arduino.h>
#include <memory>
#include <cstdio>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <Elog.h>
#include "appStati.hpp"
#include "appPrefs.hpp"
#include "statics.hpp"
#include "main.hpp"

constexpr uint64_t DELAYTIME = 750000ULL;
constexpr uint64_t FORCE_DELAYTIME = 10000000ULL;
constexpr uint64_t HARTBEATTIME = 400000ULL;
constexpr uint64_t CALIBRTIME = 255000ULL;

void setup()
{
  using namespace measure_h2o;

  // Debug Ausgabe init
  Serial.begin( 115200 );
  Serial.println( "main: program started..." );
  // sleep( 5 );
  elog.addSerialLogging( Serial, "GLO", DEBUG );  // Enable serial logging. We want only INFO or lower logleve.
  elog.log( INFO, "main: start with logging..." );
  elog.log( INFO, "main: init LEDSTRIPE..." );
  //
  // first appstati object init
  //
  elog.log( INFO, "main: init preferences..." );
  prefs::AppPrefs::init();
  //
  mLED = std::make_shared< MLED >( prefs::LED_NUM, prefs::LED_INTERNAL, NEO_GRB + NEO_KHZ800 );
  mLED->setBrightness( 128 );  // Set BRIGHTNESS  (max = 255)
  // start analog reader
  elog.log( INFO, "main: init preasure measure..." );
  PrSensor::init();
  // display init
  elog.log( INFO, "waveshare init..." );
  display = std::make_shared< MLCD >( prefs::DISPLAY_COLS, prefs::DISPLAY_ROWS, prefs::DISPLAY_SDA_PIN, prefs::DOSPLAY_SCL_PIN );
  display->init();
  // display->setCursor( 0, 0 );
  // display->send_string( "WASSERDRUCK APP" );
  // display->setCursor( 0, 1 );
  // display->send_string( "   S T A R T    " );
  //
  // watch for calibrating request
  //
  pinMode( static_cast< uint8_t >( prefs::CALIBR_REQ_PIN ), INPUT_PULLUP );
  //
  //  random init
  //
  randomSeed( analogRead( prefs::RAND_PIN ) );
  sleep( 3 );
  display->clear();
}

void loop()
{
  static uint64_t nextTimeToForceShowPresure = FORCE_DELAYTIME;
  static uint64_t nextTimeToShowPresure = DELAYTIME;
  static uint64_t nextTimeHartbeat = HARTBEATTIME;
  static uint64_t nextTimeCalibrCheck = CALIBRTIME;
  uint64_t nowTime = esp_timer_get_time();

  using namespace measure_h2o;

  if ( nowTime > nextTimeCalibrCheck )
  {
    //
    // time to check if the master whish to calibre
    //
    nextTimeCalibrCheck = nowTime + CALIBRTIME;
    auto cl_switch = digitalRead( prefs::CALIBR_REQ_PIN );
    if ( cl_switch == LOW )
    {
      //
      // there was an low impulse, is this permanent
      //
      elog.log( DEBUG, "main: Calibr requested???" );
      delay( 20 );
      //
      // after that, always low?
      //
      cl_switch = digitalRead( prefs::CALIBR_REQ_PIN );
      if ( cl_switch == LOW )
      {
        String msg( "Druck erkannt!" );
        String nix( "nicht moeglich" );
        // maybe i can calibr?
        elog.log( DEBUG, "main: Calibr requested!" );
        // is ther a preasure in the sensor?
        if ( prefs::CURRENT_BORDER_FOR_CALIBR < prefs::AppPrefs::getCurrentMiliVolts() )
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
          nextTimeCalibrCheck = nowTime + 10000000ULL;
        }
        else
        {
          //
          // i should calibr the device
          //
          elog.log( DEBUG, "main: call calibre routine..." );
          String msg( "calibriere..." );
          display->printMessage( msg );
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
  }

  if ( nowTime > nextTimeToShowPresure )
  {
    //
    // lets actualize the preasure display
    //
    nextTimeToShowPresure = nowTime + DELAYTIME;
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
    //
    // read measure values if changed
    //
    if ( prefs::AppPrefs::getWasChanged() || nowTime > nextTimeToForceShowPresure )
    {
      uint32_t mVolt = prefs::AppPrefs::getCurrentMiliVolts();
      float volt = mVolt / 1000.0f;
      float pressureBar = prefs::AppPrefs::getCurrentPressureBar();
      elog.log( DEBUG, "main: pressure raw value <%1.2f V>, pressure <%1.2f bar>", volt, pressureBar );
      //
      display->printTension( volt );
      display->printPresure( pressureBar );
      prefs::AppPrefs::resetWasChanged();
      // ever all this time
      nextTimeToForceShowPresure = nowTime + FORCE_DELAYTIME;
    }
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
}
