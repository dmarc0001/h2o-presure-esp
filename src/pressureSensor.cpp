#include <Arduino.h>
#include "statics.hpp"
#include "pressureSensor.hpp"
#include "appPrefs.hpp"
#include "appStructs.hpp"
#include "appStati.hpp"
#include "fileService.hpp"

namespace measure_h2o
{
  const char *PrSensor::tag{ "PrSensor" };
  gpio_num_t PrSensor::adcPin{ prefs::PRESSURE_GPIO };
  volatile bool PrSensor::pauseMeasureTask{ false };
  uint64_t PrSensor::interval_ys{ prefs::MEASURE_DIFF_TIME_S * 1000000ULL };

  TaskHandle_t PrSensor::taskHandle{ nullptr };

  void PrSensor::init()
  {
    elog.log( DEBUG, "%s: init pressure measure object...", PrSensor::tag );
    pinMode( PrSensor::adcPin, INPUT );
    adcAttachPin( PrSensor::adcPin );
    analogSetAttenuation( ADC_11db );
    analogReadResolution( prefs::PRESSURE_RES );
    PrSensor::interval_ys = ( prefs::AppStati::getMeasureInterval_s() * 1000000ULL );
    PrSensor::start();
    elog.log( DEBUG, "%s: init pressure measure object...OK", PrSensor::tag );
  }

  void PrSensor::start()
  {
    elog.log( INFO, "%s: Task start...", PrSensor::tag );

    if ( PrSensor::taskHandle )
    {
      vTaskDelete( PrSensor::taskHandle );
      PrSensor::taskHandle = nullptr;
    }
    else
    {
      xTaskCreate( PrSensor::mTask, "m-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY, &PrSensor::taskHandle );
    }
  }

  bool PrSensor::calibreSensor()
  {
    //
    // make measure thread sleep
    //
    PrSensor::pauseMeasureTask = true;
    // wait a bit
    taskYIELD();
    delay( 1000 );
    taskYIELD();
    //
    // read sensor value
    //
    PrSensor::doMeasure();
    //
    // set min volt == measured value
    //
    prefs::AppStati::setCalibreMinVal( prefs::AppStati::getCurrentMiliVolts() );
    //
    // compute calibre factor
    //
    // PrSensor::getCalibreFactor();
    //
    PrSensor::pauseMeasureTask = false;
    taskYIELD();
    return true;
  }

  void PrSensor::doMeasure()
  {
    //
    // measure
    // 8 times measure, ackumulate, then div 8
    //
    uint32_t readValuesSum{ 0UL };
    // set flag it was mesured
    prefs::AppStati::wasMeasure = true;
    for ( int idx = 0; idx < 8; idx++ )
    {
      // read value
      readValuesSum += analogReadMilliVolts( prefs::PRESSURE_GPIO );
      delay( 8 );
    }
    // average minus bias
    uint32_t cMiliVolts = ( readValuesSum >> 3 );
    prefs::AppStati::setCurrentMiliVolts( cMiliVolts );
    double volts = ( cMiliVolts - prefs::AppStati::getCalibreMinVal() ) / 1000.0;
    float cBar = volts * prefs::AppStati::getCalibreFactor();
    if ( cBar < 0.0F || cBar > 6.0F )
      prefs::AppStati::setCurrentPressureBar( 0.0F );
    else
      prefs::AppStati::setCurrentPressureBar( cBar );
  }

  void PrSensor::mTask( void * )
  {
    static uint64_t nextTimeToMeasure = prefs::MEASURE_DIFF_TIME_S * 1000000ULL;

    while ( true )
    {
      //
      // if an other process work here (i.e. calibre pressure)
      // or there is no tiome availible
      // make a break ;-)
      //
      while ( PrSensor::pauseMeasureTask || ( prefs::AppStati::getWlanState() != WlanState::TIMESYNCED ) )
        delay( 500U );
      //
      // normal task
      //
      if ( esp_timer_get_time() > nextTimeToMeasure )
      {
        nextTimeToMeasure = esp_timer_get_time() + PrSensor::interval_ys;
        elog.log( DEBUG, "%s: pressure measure...", PrSensor::tag );
        // show mark to message "im measuring"
        display->printMeasureMark();
        //
        // measure
        //
        PrSensor::doMeasure();
        //
        // do save
        //
        presure_data_t dataset;
        char buffer[ 24 ];
        snprintf( buffer, 22, "%04d-%02d-%02dT%02d:%02d:%02d\0", year(), month(), day(), hour(), minute(), second() );
        String tmStr( buffer );
        dataset.timestamp = buffer;
        dataset.miliVolts = prefs::AppStati::getCurrentMiliVolts();
        dataset.pressureBar = prefs::AppStati::getCurrentPressureBar();
        FileService::dataset.push_back( dataset );
        delay( 500U );
        display->hideMeasureMark();
      }
      delay( 753U );
    }
  }

}  // namespace measure_h2o