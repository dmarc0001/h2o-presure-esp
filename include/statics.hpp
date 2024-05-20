#pragma once
#include <memory>
#include <Elog.h>
#include "ledStripe.hpp"
#include "pressureSensor.hpp"
#include "lcd1602.hpp"

namespace measure_h2o
{
  extern ledObjPtr mLED;
  extern Elog elog;
  extern sysDisplay display;
}  // namespace measure_h2o