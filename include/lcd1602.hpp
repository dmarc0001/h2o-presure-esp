#pragma once
#include <memory>
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

    public:
    MLCD( uint8_t lcd_cols, uint8_t lcd_rows, int sda, int scl );
    void printPresure( float );
    void printTension( float );
    void printHartbeat();
    void printMeasureMark();
    void hideMeasureMark();
    void printAlert( String & );
    void printMessage( String & );
  };

  using sysDisplay = std::shared_ptr< MLCD >;
}  // namespace measure_h2o