#include "statics.hpp"

namespace measure_h2o
{
  // logger
  Elog elog;
  // les stripe
  ledObjPtr mLED{ nullptr };
  // pressure sensor
  pressureObjePtr prSensor;
  // system display
  sysDisplay display;

}  // namespace measure_h2o