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
      , firstLine()
      , secondLine()
  {
    elog.log( DEBUG, "%s: MLCD init...", MLCD::tag );
    Wire.setPins( _sda, _scl );
    vSemaphoreCreateBinary( displaySem );
  }

  void MLCD::init()
  {
    printedPresureTitle = false;
    printedTension = false;
    printedAlert = false;
    printedMessage = false;
    showMeasureMark = false;
    Waveshare_LCD1602::init();
    uint8_t backsl[ 8 ] = { 0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 };
    uint8_t measure[ 8 ] = { 0x0e, 0x11, 0x15, 0x15, 0x15, 0x11, 0x0e, 0x00 };
    uint8_t ant[ 8 ] = { 0x04, 0x0a, 0x15, 0x0a, 0x04, 0x04, 0x04, 0x04 };
    this->customSymbol( 0, backsl );
    this->customSymbol( 1, measure );
    this->customSymbol( 2, ant );
    this->printGreeting();
  }  // namespace measure_h2o

  void MLCD::clear()
  {
    printedPresureTitle = false;
    printedTension = false;
    printedAlert = false;
    printedMessage = false;
    showMeasureMark = false;
    Waveshare_LCD1602::clear();
  }

  void MLCD::printGreeting()
  {
    this->setCursor( 0, 0 );
    this->send_string( "WASSERDRUCK APP" );
    this->setCursor( 0, 1 );
    this->send_string( "   S T A R T    " );
  }

  void MLCD::printLine( String &_line )
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
    {
      clear();
      secondLine = firstLine;
      firstLine = _line.substring( 0, 16 );
      this->setCursor( 0, 1 );
      this->send_string( firstLine.c_str() );
      this->setCursor( 0, 0 );
      this->send_string( secondLine.c_str() );
      xSemaphoreGive( displaySem );
    }
  }

  void MLCD::printPresure( float _pressureBar )
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
    {
      if ( !printedPresureTitle )
      {
        this->setCursor( 0, 1 );
        this->send_string( "Druck:     bar  " );
        printedPresureTitle = true;
        printedAlert = false;
        printedMessage = false;
      }
      if ( lastPressure != _pressureBar )
      {
        char buffer[ 16 ];
        snprintf( buffer, 5, "%1.2f", _pressureBar );
        this->setCursor( 6, 1 );
        this->send_string( buffer );
        lastPressure = _pressureBar;
      }
    }
    xSemaphoreGive( displaySem );
  }

  void MLCD::printTension( float _tension )
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
    {
      if ( !printedTension )
      {
        this->setCursor( 0, 0 );
        this->send_string( "Spng:      V    " );
        printedTension = true;
        printedAlert = false;
        printedMessage = false;
      }
      if ( lastTension != _tension )
      {
        char buffer[ 16 ];
        this->setCursor( 6, 0 );
        snprintf( buffer, 8, "%1.2f V", _tension );
        this->send_string( buffer );
        lastTension = _tension;
      }
    }
    xSemaphoreGive( displaySem );
  }

  void MLCD::printTime( const String &_timeStr )
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
    {
      this->setCursor( 0, 0 );
      this->send_string( "Zeit:           " );
      this->setCursor( 6, 0 );
      this->send_string( _timeStr.substring( 0, 10 ).c_str() );
    }
    xSemaphoreGive( displaySem );
  }

  void MLCD::printHartbeat()
  {
    static uint8_t beat{ 0 };
    //
    if ( showMeasureMark )
      return;
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
    {
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
      }
    }
    xSemaphoreGive( displaySem );
  }

  void MLCD::printMeasureMark()
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
    {
      showMeasureMark = true;
      this->setCursor( 15, 0 );
      this->write_char( 1 );
    }
    xSemaphoreGive( displaySem );
  }

  void MLCD::hideMeasureMark()
  {
    showMeasureMark = false;
  }

  void MLCD::printAlert( String &_msg )
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
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
    xSemaphoreGive( displaySem );
  }

  void MLCD::printMessage( String &_msg )
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
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
    xSemaphoreGive( displaySem );
  }

  void MLCD::printAntMark()
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
    {
      this->setCursor( 15, 1 );
      this->write_char( 2 );
    }
    xSemaphoreGive( displaySem );
  }

  void MLCD::hideAntMark()
  {
    if ( xSemaphoreTake( displaySem, pdMS_TO_TICKS( 2000 ) ) == pdTRUE )
    {
      this->setCursor( 15, 1 );
      this->write_char( 0x20 );
    }
    xSemaphoreGive( displaySem );
  }

}  // namespace measure_h2o