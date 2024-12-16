#pragma once
#include "driver/gpio.h"
#include "version.hpp"

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
  constexpr const char *WIFI_CONFIG_AP = "esp32MeasureWiFi";                        //! accesspoint for WiFi if not config
  constexpr const char *WIFI_CONFIG_PASS = "password";                              //! AP password
  constexpr const char *APPNAME = "PRESSURE";                                       //! application name
  constexpr const char *DEFAULT_HOSTNAME = "esp_h2o";                               //! default hostname
  constexpr const char *NTP_POOL_01{ "pool.ntp.org" };                              //! ntp pool
  constexpr const char *NTP_POOL_00{ "de.pool.ntp.org" };                           //! ntp pool
  constexpr int LED_NUM = 4;                                                        //! led count
  constexpr gpio_num_t LED_PIN = GPIO_NUM_7;                                        //! LED Stripe PIN
  constexpr int LED_LAN = 0;                                                        //! LED for WLAN STATE
  constexpr int LED_TIMESYNC = 1;                                                   //! LED for Time SYNC
  constexpr int LED_MEASURESTATE = 2;                                               //! LED for measure state
  constexpr int LED_HTTP_ACTIVE = 3;                                                //! LED for http access state
  constexpr uint8_t LED_GLOBAL_BRIGHTNESS = 128;                                    //! global brightness led stripe
  constexpr uint64_t LED_CHECK_DIFF_TIME_MS = 200ULL;                               //! time between task for led sleeps
  constexpr gpio_num_t PRESSURE_GPIO = GPIO_NUM_0;                                  //! analog read pressure
  constexpr uint8_t PRESSURE_RES = 12;                                              //! resulution für current
  constexpr double PRESSURE_CALIBR_VALUE = 2.08333;                                 //! factor raw->millivolt
  constexpr uint32_t CURRENT_BORDER_FOR_CALIBR = 380;                               //! max value for calibr
  constexpr uint32_t PRESSURE_MIN_MILIVOLT = 300;                                   //! minimal milivolt 0 bar
  constexpr uint32_t PRESSURE_MAX_MILIVOLT = 2700;                                  //! maximal milivolt 5 Bar
  constexpr uint32_t MEASURE_DIFF_TIME_S = 30;                                      //! diff between two measures secounds
  constexpr gpio_num_t DISPLAY_SDA_PIN = GPIO_NUM_5;                                //! PIN SDA for I2C display
  constexpr gpio_num_t DOSPLAY_SCL_PIN = GPIO_NUM_6;                                //! PIN SCL for I2C display
  constexpr int DISPLAY_COLS = 16;                                                  //! display has 16 clumns
  constexpr int DISPLAY_ROWS = 2;                                                   //! display has 2 rows
  constexpr gpio_num_t CALIBR_REQ_PIN = GPIO_NUM_1;                                 //! if pin on ground -> calibrate
  constexpr gpio_num_t RAND_PIN = GPIO_NUM_3;                                       //! if pin on ground -> calibrate
  constexpr const char *WEB_PATH{ "/www/" };                                        //! virtual path web
  constexpr const char *DATA_PATH{ "/data/" };                                      //! virtual path data
  constexpr const char *MOUNTPOINT{ "/spiffs" };                                    //! mountpoint/makrker filesystem
  constexpr const char *WEB_PARTITION_LABEL{ "mydata" };                            //! label of the spiffs or null
  constexpr uint64_t FILE_TASK_DELAY_YS = 10ULL * 1000000ULL;                       //! delay time for saving task
  constexpr uint64_t FILE_TASK_CHECK_DELAY_YS = 7ULL * 60ULL * 60ULL * 1000000ULL;  //! delay time for check filesystem
  constexpr uint64_t FILE_SYSTEM_SIZE_CHECK_YS = 59ULL * 60ULL * 1000000ULL;        //! delay time for check filesystem ( one hour)
  constexpr size_t MIN_FILE_SYSTEM_FREE_SIZE = 300000;                              //! minimal free size on filesystem
  constexpr const char *DAYLY_FILE_NAME{ "%04d-%02d-%02d-pressure.csv" };           //! data dayly for pressure
  constexpr const char *DAYLY_FILE_PATTERN{ "^/data/\\d\\d\\d\\d-\\d\\d-\\d\\d-pressure.csv$" };  //! filename pattern
  constexpr time_t MAX_DATA_FILE_AGE_SEC = 5L * 24L * 60L * 60L;                                  //! max age for files

  //
  // LED COLORS
  // ((uint32_t)r << 16) | ((uint32_t)g << 8) | b
  //
  constexpr uint32_t LED_COLOR_BLACK = 0x00;
  constexpr uint32_t LED_COLOR_WLAN_UNKNOWN = 0x00c7825d;
  constexpr uint32_t LED_COLOR_WLAN_DISCONNECTED = 0x008a1d6f;
  constexpr uint32_t LED_COLOR_WLAN_SEARCHING = 0x00ffff00;
  constexpr uint32_t LED_COLOR_WLAN_CONNECTED = 0x0000ff00;
  constexpr uint32_t LED_COLOR_WLAN_TIME_SYNCED = 0x0000ff00;
  constexpr uint32_t LED_COLOR_WLAN_TIME_NOT_SYNCED = 0x00bd660f;
  constexpr uint32_t LED_COLOR_WLAN_ERROR = 0x00FF0000;
  constexpr uint32_t LED_COLOR_MEASURE_ACTICE = 0x00f00cd1;
  constexpr uint32_t LED_COLOR_MEASURE_INACTICE = 0x00bd660f;
  constexpr uint32_t LED_COLOR_HTTP_ACCESS = 0x00FFFFA0;

}  // namespace prefs