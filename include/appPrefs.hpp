#pragma once
#include "driver/gpio.h"

namespace prefs
{

  constexpr gpio_num_t LED_INTERNAL = GPIO_NUM_8;
  constexpr int LED_NUM = 1;
  constexpr gpio_num_t PRESSURE_GPIO = GPIO_NUM_0;
  constexpr uint8_t PRESSURE_RES = 12;
  constexpr double PRESSURE_CALIBR_VALUE = 2.08333;                    // factor raw->millivolt
  constexpr uint32_t PRESSURE_MIN_MILIVOLT = 300;                      // minimal milivolt 0 bar
  constexpr uint32_t PRESSURE_MAX_MILIVOLT = 2700;                     // maximal milivolt 5 Bar
  constexpr uint64_t PRESSURE_MEASURE_DIFF_TIME_YS = 5 * 1000 * 1000;  // diff between two measures
  constexpr int DISPLAY_COLS = 16;
  constexpr int DISPLAY_ROWS = 2;
  constexpr int DISPLAY_SDA = 5;
  constexpr int DOSPLAY_SCL = 6;

}  // namespace prefs