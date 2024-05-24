#include "lcd1602.hpp"
#include "appPrefs.hpp"
#include "statics.hpp"

namespace measure_h2o
{
  const char *MLCD::tag{ "MLCD" };

  MLCD::MLCD( uint8_t _cols, uint8_t _rows, int _sda, int _scl )
      : Waveshare_LCD1602( _cols, _rows )
      , printedPresureTitle{ false }
      , printedTension{ false }
      , printedAlert{ false }
      , printedMessage{ false }
      , showMeasureMark{ false }
  {
    elog.log( DEBUG, "%s: MLCD init...", MLCD::tag );
    Wire.setPins( _sda, _scl );
  }

  void MLCD::init()
  {
    Waveshare_LCD1602::init();
    this->setCursor( 0, 0 );
    this->send_string( "WASSERDRUCK APP" );
    this->setCursor( 0, 1 );
    this->send_string( "   S T A R T    " );
  }

  void MLCD::printPresure( float _pressureBar )
  {
    if ( !printedPresureTitle )
    {
      this->setCursor( 0, 1 );
      this->send_string( "Druck:      bar " );
      printedPresureTitle = true;
      printedAlert = false;
      printedMessage = false;
    }
    if ( lastPressure != _pressureBar )
    {
      char buffer[ 16 ];
      snprintf( buffer, 5, "%1.2f", _pressureBar );
      this->setCursor( 7, 1 );
      this->send_string( buffer );
      lastPressure = _pressureBar;
    }
  }

  void MLCD::printTension( float _tension )
  {
    if ( !printedTension )
    {
      this->setCursor( 0, 0 );
      this->send_string( "Spng:           " );
      printedTension = true;
      printedAlert = false;
      printedMessage = false;
    }
    if ( lastTension != _tension )
    {
      char buffer[ 16 ];
      this->setCursor( 7, 0 );
      snprintf( buffer, 8, "%1.2f V", _tension );
      this->send_string( buffer );
      lastTension = _tension;
    }
  }

  void MLCD::printHartbeat()
  {
    static bool beat;
    if ( showMeasureMark )
      return;
    this->setCursor( 15, 0 );
    if ( beat )
      this->write_char( '|' );
    else
      this->write_char( '-' );
    beat = !beat;
  }

  void MLCD::printMeasureMark()
  {
    showMeasureMark = true;
    this->setCursor( 15, 0 );
    this->write_char( '@' );
  }

  void MLCD::hideMeasureMark()
  {
    showMeasureMark = false;
  }

  void MLCD::printAlert( String &_msg )
  {
    printedTension = false;
    printedPresureTitle = false;
    if ( !printedAlert )
    {
      this->clear();
      this->setCursor( 0, 0 );
      this->send_string( " FEHLER:" );
      printedAlert = true;
    }
    if ( _msg )
    {
      this->setCursor( 0, 1 );
      this->send_string( _msg.c_str() );
    }
  }

  void MLCD::printMessage( String &_msg )
  {
    printedTension = false;
    printedPresureTitle = false;
    if ( !printedMessage )
    {
      this->clear();
      this->setCursor( 0, 0 );
      this->send_string( " NACHRICHT:" );
      printedMessage = true;
    }
    if ( _msg )
    {
      this->setCursor( 0, 1 );
      this->send_string( _msg.c_str() );
    }
  }

}  // namespace measure_h2o