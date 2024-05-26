#pragma once
#include <Preferences.h>
#include "appPrefs.hpp"
#include "appStructs.hpp"
#include "filesystem.hpp"

namespace prefs
{
  using namespace measure_h2o;

  class AppStati
  {
    private:
    static const char *tag;
    static bool wasInit;                     //! was the prefs object initialized?
    static bool httpActive;                  //! was http active?
    static Preferences lPref;                //! static preferences object
    static uint32_t calibreMinVal;           //! minimal value by calibr
    static uint32_t calibreMaxVal;           //! maximal value by calibr
    static double calibreFactor;             //! computed linear factor
    static uint32_t currentMiliVolts;        //! current measured value
    static float currentPressureBar;         //! current measured value
    static volatile bool presureWasChanged;  //! was presure changed?
    static WlanState wlanState;              //! is wlan disconnected, connected etc....

    public:
    static void init();
    static String getHostName();  //! get my own hostname
    static bool getIsSpiffsInit()
    {
      return Filesystem::getIsOkay();
    }
    static bool getHttpActive()
    {
      return AppStati::httpActive;
    }
    static void setHttpActive( bool _val )
    {
      AppStati::httpActive = _val;
    }
    static uint32_t getCalibreMinVal();
    static uint32_t getCalibreMaxVal();
    static double getCalibreFactor();
    static uint32_t getCurrentMiliVolts()
    {
      return AppStati::currentMiliVolts;
    }
    static float getCurrentPressureBar()
    {
      return AppStati::currentPressureBar;
    }
    //
    static void setCalibreMinVal( uint32_t );
    static void setCalibreMaxVal( uint32_t );
    static void setCalibreFactor( double );
    static void setCurrentMiliVolts( uint32_t );
    static void setCurrentPressureBar( float );
    static bool getWasChanged()
    {
      return AppStati::presureWasChanged;
    }
    static void resetWasChanged()
    {
      AppStati::presureWasChanged = false;
    }
    static void setWlanState( WlanState _state )
    {
      AppStati::wlanState = _state;
    }
    static WlanState getWlanState()
    {
      return AppStati::wlanState;
    }
    static String getTimeZone();                   //! get my timezone
    static bool setTimeZone( String & );           //! set my timezone
    static uint8_t getLogLevel();                  //! get Logging
    static bool setLogLevel( uint8_t );            //! set Logging
    static uint32_t getMeasureInterval_s();        //! get interval bwtween two measures
    static void setMeasureInterval_s( uint32_t );  //! set Interval bewtween two measures
    private:
    static bool getIfPrefsInit();        //! internal, is preferences initialized?
    static bool setIfPrefsInit( bool );  //! internal, set preferences initialized?
  };

}  // namespace prefs