#pragma once
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <Wire.h>
#include <Waveshare_LCD1602.h>

namespace measure_h2o
{
  class MLCD : public Waveshare_LCD1602
  {
    private:
    static const char *tag;
    bool printedPresureTitle;
    bool printedTension;
    bool printedAlert;
    bool printedMessage;
    bool showMeasureMark;
    float lastPressure;
    float lastTension;
    String firstLine, secondLine;
    SemaphoreHandle_t displaySem;  //! is access to files busy

    public:
    MLCD( uint8_t lcd_cols, uint8_t lcd_rows, int sda, int scl );
    void init();
    void clear();
    void printPresure( float );
    void printTension( float );
    void printTime( const String & );
    void printHartbeat();
    void printMeasureMark();
    void hideMeasureMark();
    void printAlert( String & );
    void printMessage( String & );
    void printGreeting();
    void printLine( String & );
    void printAntMark();
    void hideAntMark();
  };

  using sysDisplay = std::shared_ptr< MLCD >;
}  // namespace measure_h2o