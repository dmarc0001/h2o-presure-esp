#pragma once
#include <stdio.h>
#include <memory>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Adafruit_NeoPixel.h>
#include "appPrefs.hpp"

namespace measure_h2o
{

  class MLED : public Adafruit_NeoPixel
  {
    private:
    static const char *tag;
    MLED();

    public:
    MLED( uint16_t n, int16_t pin = 6, neoPixelType type = NEO_GRB + NEO_KHZ800 );
  };

  using ledObjPtr = std::shared_ptr< MLED >;

}  // namespace measure_h2o