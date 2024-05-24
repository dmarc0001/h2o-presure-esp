#include <Arduino.h>
#include "statics.hpp"
#include "pressureSensor.hpp"

namespace measure_h2o
{
  const char *PrSensor::tag{ "PrSensor" };
  gpio_num_t PrSensor::pin{ prefs::PRESSURE_GPIO };
  uint32_t PrSensor::calibreMinVal{ prefs::PRESSURE_MIN_MILIVOLT };
  uint32_t PrSensor::calibreMaxVal{ prefs::PRESSURE_MAX_MILIVOLT };
  double PrSensor::calibreFactor{ prefs::PRESSURE_CALIBR_VALUE };
  uint32_t PrSensor::currentMiliVolts{ 0 };
  float PrSensor::currentPressureBar{ 0.0F };
  volatile bool PrSensor::pauseMeasureTask{ false };

  TaskHandle_t PrSensor::taskHandle{ nullptr };

  void PrSensor::init()
  {
    elog.log( DEBUG, "%s: init pressure measure object...", PrSensor::tag );
    pinMode( PrSensor::pin, INPUT );
    adcAttachPin( PrSensor::pin );
    analogSetAttenuation( ADC_11db );
    analogReadResolution( prefs::PRESSURE_RES );
    PrSensor::start();
    elog.log( DEBUG, "%s: init pressure measure object...OK", PrSensor::tag );
  }

  double PrSensor::getCalibreFactor()
  {
    // auto rawMaxVal = PrSensor::calibreMaxVal - PrSensor::calibreMinVal;
    // auto maxMilivolt = prefs::PRESSURE_MAX_MILIVOLT - prefs::PRESSURE_MIN_MILIVOLT;

    // PrSensor::calibreFactor = static_cast< double >( maxMilivolt / rawMaxVal );
    return PrSensor::calibreFactor;
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
    PrSensor::calibreMinVal = PrSensor::currentMiliVolts;
    //
    // compute calibre factor
    //
    //PrSensor::getCalibreFactor();
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
    PrSensor::currentMiliVolts = ( readValuesSum >> 3 );
    PrSensor::currentPressureBar = ( ( PrSensor::currentMiliVolts - PrSensor::calibreMinVal ) / 1000.0 ) * PrSensor::calibreFactor;
    if ( PrSensor::currentPressureBar < 0.0F || PrSensor::currentPressureBar > 6.0 )
      PrSensor::currentPressureBar = 0.0F;
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