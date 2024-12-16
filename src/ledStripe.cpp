#include "appPrefs.hpp"
#include "appStati.hpp"
#include "ledStripe.hpp"
#include "statics.hpp"

namespace measure_h2o
{
  const char *MLED::tag{ "MLED" };
  Adafruit_NeoPixel MLED::ledStripe;
  TaskHandle_t MLED::taskHandle{ nullptr };

  const uint32_t checkColors[] PROGMEM = { 0x00ff0000UL, 0x00ffff00UL, 0x0000ff00UL, 0x0000ffffUL, 0x000000ffUL, 0x00ffffffUL, 0x0UL };

  /**
   * init the static object
   */
  void MLED::init( uint16_t _countLed, int16_t _gpio, neoPixelType _type )
  {
    elog.log( DEBUG, "%s: init MLED...", MLED::tag );
    ledStripe.clear();
    ledStripe.setPin( _gpio );
    ledStripe.updateType( _type );
    ledStripe.updateLength( _countLed );
    ledStripe.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
    ledStripe.show();   // Turn OFF all pixels ASAP
    ledStripe.setBrightness( prefs::AppStati::getLedBrightness() );
    MLED::start();
    elog.log( DEBUG, "%s: init MLED...OK", MLED::tag );
  }

  /**
   * start the LEd task
   */
  void MLED::start()
  {
    elog.log( INFO, "%s: start Task...", MLED::tag );

    //
    // functional check
    //
    ledStripe.setBrightness( 255U );
    for ( auto j = 0; j < 7; ++j )
    {
      uint32_t col = checkColors[ j ];
      for ( auto i = 0; i < ledStripe.numPixels(); i++ )
      {
        ledStripe.setPixelColor( i, col );
        ledStripe.show();
        delay( 60 );
      }
    }
    ledStripe.setBrightness( prefs::AppStati::getLedBrightness() );
    ledStripe.setPixelColor( prefs::LED_MEASURESTATE, prefs::LED_COLOR_MEASURE_INACTICE );
    ledStripe.setPixelColor( prefs::LED_TIMESYNC, prefs::LED_COLOR_WLAN_TIME_NOT_SYNCED );
    ledStripe.setPixelColor( prefs::LED_LAN, prefs::LED_COLOR_WLAN_DISCONNECTED );
    ledStripe.show();

    if ( MLED::taskHandle )
    {
      vTaskDelete( MLED::taskHandle );
      MLED::taskHandle = nullptr;
    }
    else
    {
      xTaskCreate( MLED::lTask, "l-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY, &MLED::taskHandle );
    }
  }

  /**
   * the LED Task, run forever :-)
   */
  void MLED::lTask( void * )
  {
    static uint64_t nextTimeToCheck = prefs::LED_CHECK_DIFF_TIME_MS * 1000ULL;
    static uint64_t nextTimeToMeasureLED = prefs::LED_CHECK_DIFF_TIME_MS * 1000ULL;
    static uint64_t nextTimeToHTTPLED = prefs::LED_CHECK_DIFF_TIME_MS * 1000ULL;
    static WlanState wlState = WlanState::DISCONNECTED;
    static bool measureShow{ false };
    static bool httpStateShow{ false };
    static bool configPortalMarker{ false };

    elog.log( INFO, "%s: LED Task started...", MLED::tag );
    uint64_t now = esp_timer_get_time();

    while ( true )
    {
      now = esp_timer_get_time();
      //
      if ( now > nextTimeToCheck )
      {
        nextTimeToCheck = now + ( prefs::LED_CHECK_DIFF_TIME_MS * 1000ULL );
        //
        // if config portal running
        //
        if ( prefs::AppStati::getWlanState() == CONFIGPORTAL )
        {
          if ( configPortalMarker )
          {
            ledStripe.setPixelColor( prefs::LED_LAN, prefs::LED_COLOR_WLAN_ERROR );
            ledStripe.setPixelColor( prefs::LED_HTTP_ACTIVE, prefs::LED_COLOR_WLAN_ERROR );
            ledStripe.setPixelColor( prefs::LED_TIMESYNC, prefs::LED_COLOR_BLACK );
            ledStripe.setPixelColor( prefs::LED_MEASURESTATE, prefs::LED_COLOR_BLACK );
          }
          else
          {
            ledStripe.setPixelColor( prefs::LED_LAN, prefs::LED_COLOR_BLACK );
            ledStripe.setPixelColor( prefs::LED_HTTP_ACTIVE, prefs::LED_COLOR_BLACK );
            ledStripe.setPixelColor( prefs::LED_TIMESYNC, prefs::LED_COLOR_WLAN_ERROR );
            ledStripe.setPixelColor( prefs::LED_MEASURESTATE, prefs::LED_COLOR_WLAN_ERROR );
          }
          configPortalMarker = !configPortalMarker;
        }

        //
        // time to check if LED have to change
        //
        if ( prefs::AppStati::getWlanState() != wlState )
        {
          wlState = prefs::AppStati::getWlanState();
          //
          // make WLAN state
          //
          // constexpr uint32_t LED_WLAN_UNKNOWN = 0x00c7825d;
          // constexpr uint32_t LED_WLAN_DISCONNECTED = 0x008a1d6f;
          // constexpr uint32_t LED_WLAN_SEARCHING = 0x00ffff00;
          // constexpr uint32_t LED_WLAN_CONNECTED = 0x0092ab6d;
          // constexpr uint32_t LED_WLAN_TIMESYNCED = 0x0000ff00;
          // constexpr uint32_t LED_WLAN_ERROR = 0x00FF0000;
          //
          switch ( wlState )
          {
            case DISCONNECTED:
              ledStripe.setPixelColor( prefs::LED_LAN, prefs::LED_COLOR_WLAN_DISCONNECTED );
              ledStripe.setPixelColor( prefs::LED_TIMESYNC, prefs::LED_COLOR_WLAN_TIME_NOT_SYNCED );
              break;
            case SEARCHING:
              ledStripe.setPixelColor( prefs::LED_LAN, prefs::LED_COLOR_WLAN_SEARCHING );
              ledStripe.setPixelColor( prefs::LED_TIMESYNC, prefs::LED_COLOR_WLAN_TIME_NOT_SYNCED );
              break;
            case CONNECTED:
              ledStripe.setPixelColor( prefs::LED_LAN, prefs::LED_COLOR_WLAN_CONNECTED );
              ledStripe.setPixelColor( prefs::LED_TIMESYNC, prefs::LED_COLOR_WLAN_TIME_NOT_SYNCED );
              break;
            case TIMESYNCED:
              ledStripe.setPixelColor( prefs::LED_LAN, prefs::LED_COLOR_WLAN_CONNECTED );
              ledStripe.setPixelColor( prefs::LED_TIMESYNC, prefs::LED_COLOR_WLAN_TIME_SYNCED );
              break;
            case CONFIGPORTAL:
            case FAILED:
            default:
              ledStripe.setPixelColor( prefs::LED_LAN, prefs::LED_COLOR_WLAN_ERROR );
              ledStripe.setPixelColor( prefs::LED_TIMESYNC, prefs::LED_COLOR_WLAN_ERROR );
              break;
          }
        }
        if ( prefs::AppStati::wasMeasure )
        {
          //
          // if measure was active
          //
          measureShow = true;
          prefs::AppStati::wasMeasure = false;
          ledStripe.setPixelColor( prefs::LED_MEASURESTATE, prefs::LED_COLOR_MEASURE_ACTICE );
          nextTimeToMeasureLED = now + ( prefs::LED_CHECK_DIFF_TIME_MS * 1000ULL * 3ULL );
        }
        if ( prefs::AppStati::httpActive )
        {
          httpStateShow = true;
          prefs::AppStati::httpActive = false;
          ledStripe.setPixelColor( prefs::LED_HTTP_ACTIVE, prefs::LED_COLOR_HTTP_ACCESS );
          nextTimeToHTTPLED = now + ( prefs::LED_CHECK_DIFF_TIME_MS * 500ULL );
        }
        ledStripe.show();
      }
      else if ( now > nextTimeToMeasureLED && measureShow )
      {
        //
        // time to check if measure led have to clean
        //
        measureShow = false;
        if ( wlState != TIMESYNCED )
          ledStripe.setPixelColor( prefs::LED_MEASURESTATE, prefs::LED_COLOR_MEASURE_INACTICE );
        else
          ledStripe.setPixelColor( prefs::LED_MEASURESTATE, prefs::LED_COLOR_BLACK );
        ledStripe.show();
      }
      else if ( now > nextTimeToHTTPLED && httpStateShow )
      {
        httpStateShow = false;
        ledStripe.setPixelColor( prefs::LED_HTTP_ACTIVE, prefs::LED_COLOR_BLACK );
        ledStripe.show();
      }
      else
      {
        delay( 26 );
      }
    }
  }

}  // namespace measure_h2o