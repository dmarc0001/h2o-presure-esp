#pragma once
#include "driver/gpio.h"

namespace prefs
{

//
// different models has different pins for internal LED
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
  constexpr int LED_NUM = 1;                                           // only one LED internal
  constexpr gpio_num_t PRESSURE_GPIO = GPIO_NUM_0;                     // analog read pressure
  constexpr uint8_t PRESSURE_RES = 12;                                 // resulution für current
  constexpr double PRESSURE_CALIBR_VALUE = 2.08333;                    // factor raw->millivolt
  constexpr uint32_t PRESSURE_MIN_MILIVOLT = 300;                      // minimal milivolt 0 bar
  constexpr uint32_t PRESSURE_MAX_MILIVOLT = 2700;                     // maximal milivolt 5 Bar
  constexpr uint64_t PRESSURE_MEASURE_DIFF_TIME_YS = 5 * 1000 * 1000;  // diff between two measures
  constexpr int DISPLAY_SDA_PIN = 5;                                   // PIN SDA for I2C display
  constexpr int DOSPLAY_SCL_PIN = 6;                                   // PIN SCL for I2C display
  constexpr int DISPLAY_COLS = 16;                                     // display has 16 clumns
  constexpr int DISPLAY_ROWS = 2;                                      // display has 2 rows

}  // namespace prefs