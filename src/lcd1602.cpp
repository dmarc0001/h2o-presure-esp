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
    uint8_t backsl[ 8 ] = { 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 };
    uint8_t measure[ 8 ] = { 0x0e, 0x11, 0x15, 0x15, 0x15, 0x11, 0x0e, 0x00 };
    this->customSymbol( 0, backsl );
    this->customSymbol( 1, measure );
    this->setCursor( 0, 0 );
    this->send_string( "WASSERDRUCK APP" );
    this->setCursor( 0, 1 );
    this->send_string( "   S T A R T    " );
  }  // namespace measure_h2o

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
      this->send_string( "Spng:       V   " );
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
    static uint8_t beat{ 0 };
    //
    if ( showMeasureMark )
      return;
    this->setCursor( 15, 0 );
    ++beat;
    switch ( beat )
    {
      case 1:
        this->write_char( '|' );
        break;

      case 2:
        this->write_char( '/' );
        break;

      case 3:
        this->write_char( '-' );
        break;

      case 4:
      default:
        this->write_char( 0 );
        beat = 0;
        break;

        // case 4:
        // this->write_char( '|' );
        // break;

        // case 5:
        // this->write_char( '/' );
        // break;

        // case 6:
        // this->write_char( '-' );
        // break;

        // case 7:
        // this->write_char( '\\' );
        // break;
    }
  }

  void MLCD::printMeasureMark()
  {
    showMeasureMark = true;
    this->setCursor( 15, 0 );
    this->write_char( 1 );
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