#include "lcd1602.hpp"
#include "appPrefs.hpp"
#include "statics.hpp"

namespace measure_h2o
{
  const char *MLCD::tag{ "MLCD" };

  MLCD::MLCD( uint8_t _cols, uint8_t _rows, int _sda, int _scl ) : Waveshare_LCD1602( _cols, _rows )
  {
    elog.log( DEBUG, "%s: MLCD init...", MLCD::tag );
    Wire.setPins( _sda, _scl );
  }

}  // namespace measure_h2o