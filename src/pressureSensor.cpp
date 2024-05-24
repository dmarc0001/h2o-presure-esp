#include <Arduino.h>
#include "statics.hpp"
#include "pressureSensor.hpp"
#include "appPrefs.hpp"
#include "appStati.hpp"

namespace measure_h2o
{
  const char *PrSensor::tag{ "PrSensor" };
  gpio_num_t PrSensor::adcPin{ prefs::PRESSURE_GPIO };
  volatile bool PrSensor::pauseMeasureTask{ false };

  TaskHandle_t PrSensor::taskHandle{ nullptr };

  void PrSensor::init()
  {
    elog.log( DEBUG, "%s: init pressure measure object...", PrSensor::tag );
    pinMode( PrSensor::adcPin, INPUT );
    adcAttachPin( PrSensor::adcPin );
    analogSetAttenuation( ADC_11db );
    analogReadResolution( prefs::PRESSURE_RES );
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
    prefs::AppPrefs::setCalibreMinVal( prefs::AppPrefs::getCurrentMiliVolts() );
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
    for ( int idx = 0; idx < 8; idx++ )
    {
      // read value
      readValuesSum += analogReadMilliVolts( prefs::PRESSURE_GPIO );
      delay( 10 );
    }
    // average minus bias
    uint32_t cMiliVolts = ( readValuesSum >> 3 );
    prefs::AppPrefs::setCurrentMiliVolts( cMiliVolts );
    double volts = ( cMiliVolts - prefs::AppPrefs::getCalibreMinVal() ) / 1000.0;
    elog.log( DEBUG, "%s: current: <%02.2fV> - factor <%02.5f>", PrSensor::tag, static_cast< float >( volts ),
              static_cast< float >( prefs::AppPrefs::getCalibreFactor() ) );
    float cBar = volts * prefs::AppPrefs::getCalibreFactor();
    if ( cBar < 0.0F || cBar > 6.0F )
      prefs::AppPrefs::setCurrentPressureBar( 0.0F );
    else
      prefs::AppPrefs::setCurrentPressureBar( cBar );
  }

  void PrSensor::mTask( void * )
  {
    uint64_t nextTimeToMeasure = prefs::PRESSURE_MEASURE_DIFF_TIME_YS;

    while ( true )
    {
      //
      // if an other process work here (i.e. calibre pressure)
      // make a break ;-)
      //
      while ( PrSensor::pauseMeasureTask )
        delay( 500U );
      //
      // normal task
      //
      if ( esp_timer_get_time() > nextTimeToMeasure )
      {
        nextTimeToMeasure = esp_timer_get_time() + prefs::PRESSURE_MEASURE_DIFF_TIME_YS;
        elog.log( DEBUG, "%s: pressure measure...", PrSensor::tag );
        // show mark to message "im measuring"
        display->printMeasureMark();
        //
        // measure
        //
        PrSensor::doMeasure();
        delay( 500U );
        display->hideMeasureMark();
      }
      delay( 1000U );
    }
  }

}  // namespace measure_h2o