#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp32-hal-adc.h>
#include "appPrefs.hpp"

namespace measure_h2o
{

  class PrSensor
  {
    private:
    static const char *tag;                 //! Tag for debug and messages
    static gpio_num_t adcPin;               //! gpio pin
    static TaskHandle_t taskHandle;         //! only one times
    static volatile bool pauseMeasureTask;  //! if i make an calibration, pause task
    static uint64_t interval_ys;            //! interval between two measures

    public:
    static void init();           //! init the startic object
    static bool calibreSensor();  //! calibre sensor

    private:
    static void start();          //! start measure thread
    static void mTask( void * );  //! the task fir LED
    static void doMeasure();      //! make a measure
  };

  using pressureObjePtr = std::shared_ptr< PrSensor >;

}  // namespace measure_h2o