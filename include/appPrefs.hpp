#pragma once
#include "driver/gpio.h"

namespace prefs
{

//
//! different models has different pins for internal LED
//
#ifdef LED_PIN_8
  constexpr gpio_num_t LED_INTERNAL = GPIO_NUM_8;
#else
#ifdef LED_PIN_10
  constexpr gpio_num_t LED_INTERNAL = GPIO_NUM_10;
#else
  constexpr gpio_num_t LED_INTERNAL = GPIO_NUM_21;
#endif
#endif
  constexpr const char *APPNAME = "PRESSURE";                                                    //! application name
  constexpr const char *DEFAULT_HOSTNAME = "esp_h2o_";                                           //! default hostname
  constexpr int LED_NUM = 1;                                                                     //! only one LED internal
  constexpr gpio_num_t PRESSURE_GPIO = GPIO_NUM_0;                                               //! analog read pressure
  constexpr uint8_t PRESSURE_RES = 12;                                                           //! resulution für current
  constexpr double PRESSURE_CALIBR_VALUE = 2.08333;                                              //! factor raw->millivolt
  constexpr uint32_t CURRENT_BORDER_FOR_CALIBR = 380;                                            //! max value for calibr
  constexpr uint32_t PRESSURE_MIN_MILIVOLT = 300;                                                //! minimal milivolt 0 bar
  constexpr uint32_t PRESSURE_MAX_MILIVOLT = 2700;                                               //! maximal milivolt 5 Bar
  constexpr uint64_t MEASURE_DIFF_TIME_YS = 10 * 1000 * 1000;                                    //! diff between two measures
  constexpr gpio_num_t DISPLAY_SDA_PIN = GPIO_NUM_5;                                             //! PIN SDA for I2C display
  constexpr gpio_num_t DOSPLAY_SCL_PIN = GPIO_NUM_6;                                             //! PIN SCL for I2C display
  constexpr int DISPLAY_COLS = 16;                                                               //! display has 16 clumns
  constexpr int DISPLAY_ROWS = 2;                                                                //! display has 2 rows
  constexpr gpio_num_t CALIBR_REQ_PIN = GPIO_NUM_1;                                              //! if pin on ground -> calibrate
  constexpr gpio_num_t RAND_PIN = GPIO_NUM_3;                                                    //! if pin on ground -> calibrate
  constexpr const char *WEB_PATH{ "/web" };                                                      //! virtual path web
  constexpr const char *DATA_PATH{ "/data" };                                                    //! virtual path data
  constexpr const char *WEB_PARTITION_LABEL{ "mydata" };                                         //! label of the spiffs or null
  constexpr uint64_t FILE_TASK_DELAY_YS = 2 * 1000 * 1000;                                       //! delay time for saving task
  constexpr uint64_t FILE_TASK_CHECK_DELAY_YS = 45 * 1000 * 1000;                                //! delay time for check filesystem
  constexpr const char *DAYLY_FILE_NAME{ "%04d-%02d-%02d-pressure.csv" };                        //! data dayly for pressure
  constexpr const char *DAYLY_FILE_PATTERN{ "^/data/\\d\\d\\d\\d-\\d\\d-\\d\\d-presure.csv$" };  //! filename pattern
  constexpr time_t MAX_DATA_FILE_AGE_SEC = 5 * 24 * 60 * 60;                                     //! max age for files
}  // namespace prefs