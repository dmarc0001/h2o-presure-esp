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
    static const char *tag;          //! name of the module for debug
    static bool wasInit;             //! was the prefs object initialized?
    static TaskHandle_t taskHandle;  //! only one times
    static String todayFileName;     //! todays file name
    static int todayDay;             //! todays day number

    public:
    static SemaphoreHandle_t measureFileSem;  //! is access to files busy
    static presure_data_set_t dataset;        //! set of mesures

    public:
    static void init();
    static String &getTodayFileName();
    static bool deleteTodayFile();

    private:
    static void start();
    static void sTask( void * );
    static int saveDatasets();
    static int checkFileSys();
  };
}  // namespace measure_h2o
