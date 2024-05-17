#include <Arduino.h>
#include "statics.hpp"
#include "pressureSensor.hpp"

namespace measure_h2o
{
  const char *PrSensor::tag{ "PrSensor" };

  PrSensor::PrSensor( gpio_num_t _pin, adc_attenuation_t _att, uint8_t _res )
      : pin( _pin )
      , calibrMinVal( prefs::PRESSURE_MIN_MILIVOLT )
      , calibrMaxVal( prefs::PRESSURE_MAX_MILIVOLT )
      , calibrFactor( prefs::PRESSURE_CALIBR_VALUE )
  {
    elog.log( DEBUG, "%s: constructor pressure measure object...", PrSensor::tag );
    pinMode( _pin, INPUT );
    adcAttachPin( _pin );
    analogSetAttenuation( _att );
    analogReadResolution( _res );
    elog.log( DEBUG, "%s: constructor pressure measure object...OK", PrSensor::tag );
  }

  float PrSensor::getPressureBar()
  {
    float pressureBar = 0.0;
    auto miliVolt = analogReadMilliVolts( pin ) - calibrMinVal;
    pressureBar = ( miliVolt / 1000.0 ) * calibrFactor;
    if ( pressureBar < 0.0F || pressureBar > 6.0 )
      return 0.0F;
    return pressureBar;
  }

  uint16_t PrSensor::getRawVal()
  {
    return analogRead( pin );
  }

  uint32_t PrSensor::getMilivolts()
  {
    return analogReadMilliVolts( pin );
  }

  double PrSensor::getCalibrFactor()
  {
    auto rawMaxVal = calibrMaxVal - calibrMinVal;
    auto maxMilivolt = prefs::PRESSURE_MAX_MILIVOLT - prefs::PRESSURE_MIN_MILIVOLT;

    calibrFactor = static_cast< double >( maxMilivolt / rawMaxVal );
    return calibrFactor;
  }
}  // namespace measure_h2o