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
    bool printedPresureTitle;      //! was presure text displayed?
    bool printedTension;           //! was tension prited?
    bool printedAlert;             //! was alert printed?
    bool printedMessage;           //! was message printed=
    bool showMeasureMark;          //! was indicator for measuring printed?
    float lastPressure;            //! history last pressure
    float lastTension;             //! history, last tension
    String firstLine, secondLine;  //! text in first and second line
    SemaphoreHandle_t displaySem;  //! is access to display busy

    public:
    MLCD( uint8_t lcd_cols, uint8_t lcd_rows, int sda, int scl );  //! constructor, of course
    void init();                                                   //! init this  object
    void clear();                                                  //! clear the display
    void printPresure( float );                                    //! print current pressure
    void printTension( float );                                    //! print current tension
    void printTime( const String & );                              //! print current time
    void printHartbeat();                                          //! print heartbeat
    void printMeasureMark();                                       //! print indicator that the app is measuring
    void hideMeasureMark();                                        //! hide indicator
    void printAlert( String & );                                   //! print an alert message
    void printMessage( String & );                                 //! print a common message
    void printGreeting();                                          //! print greeting message
    void printLine( String & );                                    //! print a single line
    void printAntMark();                                           //! print a sign für WiFi connection
    void hideAntMark();                                            //! hide a sign für WiFi connection
  };

  using sysDisplay = std::shared_ptr< MLCD >;
}  // namespace measure_h2o