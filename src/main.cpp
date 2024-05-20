#include <stdio.h>
#include <Arduino.h>
#include <memory>
#include <cstdio>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <Elog.h>
#include "appPrefs.hpp"
#include "statics.hpp"
#include "main.hpp"

void setup()
{
  using namespace measure_h2o;

  // Debug Ausgabe init
  Serial.begin( 115200 );
  Serial.println( "main: program started..." );
  elog.addSerialLogging( Serial, "GLO", DEBUG );  // Enable serial logging. We want only INFO or lower logleve.
  elog.log( INFO, "main: start with logging..." );
  elog.log( INFO, "main: init LEDSTRIPE..." );
  mLED = std::make_shared< MLED >( prefs::LED_NUM, prefs::LED_INTERNAL, NEO_GRB + NEO_KHZ800 );
  mLED->setBrightness( 128 );  // Set BRIGHTNESS  (max = 255)
  // start analog reader
  elog.log( INFO, "main: init preasure measure..." );
  PrSensor::init();
  // display init
  elog.log( INFO, "waveshare init..." );
  display = std::make_shared< MLCD >( prefs::DISPLAY_COLS, prefs::DISPLAY_ROWS, prefs::DISPLAY_SDA, prefs::DOSPLAY_SCL );
  display->init();
  display->setCursor( 0, 0 );
  display->send_string( "WASSERDRUCK APP" );
  display->setCursor( 0, 1 );
  display->send_string( "S T A R T ..." );

  //  random init
  randomSeed( analogRead( 1 ) );
  sleep( 3 );
  display->clear();
}

void loop()
{
  using namespace measure_h2o;
  mLED->setBrightness( static_cast< uint8_t >( 0xff & random( 0x20, 0xff ) ) );
  uint8_t red = static_cast< uint8_t >( 0xff & random( 255 ) );
  uint8_t green = static_cast< uint8_t >( 0xff & random( 255 ) );
  uint8_t blue = static_cast< uint8_t >( 0xff & random( 255 ) );

  // elog.log( DEBUG, "main: <r:%03d>, <g:%03d>, <b:%03d>", red, green, blue );
  mLED->setPixelColor( 0, mLED->Color( red, green, blue ) );
  mLED->show();
  delay( 150 );
  mLED->setPixelColor( 0, 0x0 );
  mLED->show();
  //
  // read analog raw
  //
  uint32_t mVolt = PrSensor::getMilivolts();
  float volt = mVolt / 1000.0f;
  float pressureBar = PrSensor::getPressureBar();
  elog.log( DEBUG, "main: pressure raw value <%1.2f V>, pressure <%1.2f bar>", volt, pressureBar );
  //
  display->printTension( volt );
  display->printPresure( pressureBar );
  //
  // hartbeat
  //
  display->printHartbeat();
  //
  delay( 1000 );
}
