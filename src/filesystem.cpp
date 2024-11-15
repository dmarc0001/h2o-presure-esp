#include <SPIFFS.h>
#include <Arduino.h>
#include "filesystem.hpp"
#include "appPrefs.hpp"

namespace measure_h2o
{
  bool Filesystem::wasInit;
  bool Filesystem::isOkay;

 /**
  * init the static object 
  */
  void Filesystem::init()
  {
    if ( !Filesystem::wasInit )
    {
      Serial.println( "main: init filesystem..." );
      if ( !SPIFFS.begin( false, prefs::MOUNTPOINT, 10, prefs::WEB_PARTITION_LABEL ) )
      {
        // TODO: Errormessage
        Serial.println( "main:: init failed, FORMAT filesystem..." );
        if ( !SPIFFS.format() )
        {
          // there is an error BAD!
          // TODO: Errormessage
          Serial.println( "main: An Error has occurred while mounting SPIFFS!" );
          delay( 5000 );
        }
        else
        {
          Serial.println( "main: FORMAT filesystem successful..." );
          // is okay
          Filesystem::wasInit = true;
        }
        ESP.restart();
      }
      else
      {
        // is okay
        Filesystem::isOkay = true;
        Serial.println( "main: init filesystem...OK" );
      }
      Filesystem::wasInit = true;
    }
  }
}  // namespace measure_h2o
