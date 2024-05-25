#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <SPIFFS.h>
#include "appPrefs.hpp"
#include "appStructs.hpp"

namespace measure_h2o
{
  class FileService
  {
    private:
    static const char *tag;
    static bool wasInit;                      //! was the prefs object initialized?
    static TaskHandle_t taskHandle;           //! only one times
    static SemaphoreHandle_t measureFileSem;  //! is access to files busy

    public:
    static presure_data_set_t dataset;  //! set of mesures

    public:
    static void init();

    private:
    static void start();
    static void sTask( void * );
    static int saveDatasets();
    static int checkFileSys();
    
  };
}  // namespace measure_h2o
