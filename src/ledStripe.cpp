#include "appPrefs.hpp"
#include "ledStripe.hpp"
#include "statics.hpp"

namespace measure_h2o
{
  const char *MLED::tag{ "MLED" };

  MLED::MLED( uint16_t _countLed, int16_t _gpio, neoPixelType _type ) : Adafruit_NeoPixel( _countLed, _gpio, _type )
  {
    elog.log( DEBUG, "%s: constructor MLED...", MLED::tag );
    Adafruit_NeoPixel::begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
    Adafruit_NeoPixel::show();   // Turn OFF all pixels ASAP
  }

}  // namespace measure_h2o