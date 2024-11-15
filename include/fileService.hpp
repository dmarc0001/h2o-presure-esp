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
    static void init();                 //! init the static object
    static String &getTodayFileName();  //! get the filename for today
    static bool deleteTodayFile();      //! delete the file from today

    private:
    static void start();                  //! init the task
    static void sTask( void * );          //! the static task in thes object
    static int saveDatasets();            //! save datasets from queue to file
    static int checkFileSys();            //! check if the data have to care
    static int deleteOtherThanCurrent();  //! emergency delete all other than current files
    static int checkFileSysSizes();       //! check if enough free memory
    static int computeFilesystemCheck();  //! do all the checks
  };
}  // namespace measure_h2o
