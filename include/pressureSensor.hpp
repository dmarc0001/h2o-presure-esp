#pragma once
#include <memory>
#include <esp32-hal-adc.h>
#include "appPrefs.hpp"

namespace measure_h2o
{
  class PrSensor
  {
    private:
    static const char *tag;
    gpio_num_t pin;
    uint32_t calibrMinVal;
    uint32_t calibrMaxVal;
    double calibrFactor;
    PrSensor();

    public:
    PrSensor( gpio_num_t _pin, adc_attenuation_t _att = ADC_11db, uint8_t _res = prefs::PRESSURE_RES );
    uint16_t getRawVal();
    uint32_t getMilivolts();
    float getPressureBar();
    uint16_t getMinPresureValue()
    {
      return calibrMinVal;
    }
    double getCalibrFactor();
  };

  using pressureObjePtr = std::shared_ptr< PrSensor >;

}  // namespace measure_h2o