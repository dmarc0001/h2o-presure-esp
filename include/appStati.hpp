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
    static const char *tag;                  //! logging tag
    static bool wasInit;                     //! was the prefs object initialized?
    static Preferences lPref;                //! static preferences object
    static uint32_t calibreMinVal;           //! minimal value by calibr
    static uint32_t calibreMaxVal;           //! maximal value by calibr
    static double calibreFactor;             //! computed linear factor
    static uint32_t currentMiliVolts;        //! current measured value
    static float currentPressureBar;         //! current measured value
    static volatile bool presureWasChanged;  //! was presure changed?
    static WlanState wlanState;              //! is wlan disconnected, connected etc....
    static size_t fsTotalSpace;              //! total space in FS
    static size_t fsUsedSpace;               //! free Space in FS
    static bool forceFilesystemCheck;        //! force an filesystemcheck from user initiated

    public:
    static volatile bool wasMeasure;  //! system was measuring
    static volatile bool httpActive;  //! was http active?

    public:
    static void init();
    static String getHostName();   //! get my own hostname
    static bool getIsSpiffsInit()  //! is SPIFFS initialized?
    {
      return Filesystem::getIsOkay();
    }
    static uint32_t getCalibreMinVal();    //! get minimal value
    static uint32_t getCalibreMaxVal();    //! get the minimal value for sensor meaning "no pressure" or "pressure == 0"
    static double getCalibreFactor();      //! factor for comuting the "real" pressure
    static uint32_t getCurrentMiliVolts()  //! current measured tension from AD Chip
    {
      return AppStati::currentMiliVolts;
    }
    static float getCurrentPressureBar()  //! get the current measured pressure from sensor
    {
      return AppStati::currentPressureBar;
    }
    //
    static void setCalibreMinVal( uint32_t );     //! set the value for pressure == 0
    static void setCalibreMaxVal( uint32_t );     //! set the value for pressure == max
    static void setCalibreFactor( double );       //! set the factor for computing the "real" preasure
    static void setCurrentMiliVolts( uint32_t );  //! set the current tension
    static void setCurrentPressureBar( float );   //! set the current pressure
    static bool getWasChanged()                   //! was the pressure changed since last measure
    {
      return AppStati::presureWasChanged;
    }
    static void resetWasChanged()  //! reset changed indicator
    {
      AppStati::presureWasChanged = false;
    }
    static void setWlanState( WlanState _state )  //! set the state of the WLAN
    {
      AppStati::wlanState = _state;
    }
    static WlanState getWlanState()  //! get the stsate of the WLAN
    {
      return AppStati::wlanState;
    }
    static void setFsTotalSpace( size_t _size )  //! set total space of my Flash
    {
      AppStati::fsTotalSpace = _size;
    }
    static size_t getFsTotalSpace()  //! get the total space of the flash
    {
      return AppStati::fsTotalSpace;
    }
    static void setFsUsedSpace( size_t _size )  //! set used flash size
    {
      AppStati::fsUsedSpace = _size;
    }
    static size_t getFsUsedSpace()  //! get the used flash size
    {
      return AppStati::fsUsedSpace;
    }
    static size_t getFsFreeSize()  //! comuute and return the free size
    {
      return ( AppStati::fsTotalSpace - AppStati::fsUsedSpace );
    }
    static String getTimeZone();                      //! get my timezone
    static bool setTimeZone( const String & );        //! set my timezone
    static bool setTimezoneOffset( long );            //! set timezione offst instread of timezone
    static long getTimezoneOffset();                  //! get the offset for timezone
    static uint8_t getLogLevel();                     //! get Logging
    static bool setLogLevel( uint8_t );               //! set Logging
    static uint32_t getMeasureInterval_s();           //! get interval bwtween two measures
    static bool setMeasureInterval_s( uint32_t );     //! set Interval bewtween two measures
    static uint8_t getLedBrightness();                //! get led ground brightness
    static bool setLedBrightness( uint8_t );          //! set led ground brightness
    static void setForceFilesystemCheck( bool _set )  //! set / unset force an filesystem check
    {
      AppStati::forceFilesystemCheck = _set;
    }
    static bool getForceFilesystemCheck()  // # ask for forced filesystemchcek
    {
      return AppStati::forceFilesystemCheck;
    }

    private:
    static bool getIfPrefsInit();        //! internal, is preferences initialized?
    static bool setIfPrefsInit( bool );  //! internal, set preferences initialized?
  };

}  // namespace prefs