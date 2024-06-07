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

  class MLED  //: public Adafruit_NeoPixel
  {
    private:
    static const char *tag;
    static Adafruit_NeoPixel ledStripe;
    static TaskHandle_t taskHandle;  //! only one times

    public:
    static void init( uint16_t n, int16_t pin = 6, neoPixelType type = NEO_GRB + NEO_KHZ800 );
    static void start();  //! start measure thread

    private:
    static void lTask( void * );  //! the task fuer LED
  };

}  // namespace measure_h2o