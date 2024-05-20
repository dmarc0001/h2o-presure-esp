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
    static const char *tag;            //! Tag for debug and messages
    static gpio_num_t pin;             //! gpio pin
    static uint32_t calibrMinVal;      //! minimal value by calibr
    static uint32_t calibrMaxVal;      //! maximal value by calibr
    static double calibrFactor;        //! computed linear factor
    static TaskHandle_t taskHandle;    //! only one times
    static uint32_t currentMiliVolts;  //! current measured value
    static float currentPressureBar;   //! current measured value

    public:
    static void init();
    static uint32_t getMilivolts()
    {
      return PrSensor::currentMiliVolts;
    }

    static float getPressureBar()
    {
      return PrSensor::currentPressureBar;
    }

    static uint16_t getMinPresureValue()
    {
      return calibrMinVal;
    }
    static double getCalibrFactor();

    private:
    static void start();          //! start measure thread
    static void mTask( void * );  //! the task fir LED
  };

  using pressureObjePtr = std::shared_ptr< PrSensor >;

}  // namespace measure_h2o