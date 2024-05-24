#pragma once
#include <Preferences.h>
#include "appPrefs.hpp"

namespace prefs
{
  class AppPrefs
  {
    private:
    static const char *tag;
    static bool wasInit;                     //! was the prefs object initialized?
    static Preferences lPref;                //! static preferences object
    static uint32_t calibreMinVal;           //! minimal value by calibr
    static uint32_t calibreMaxVal;           //! maximal value by calibr
    static double calibreFactor;             //! computed linear factor
    static uint32_t currentMiliVolts;        //! current measured value
    static float currentPressureBar;         //! current measured value
    static volatile bool presureWasChanged;  //! was presure changed?

    public:
    static void init();
    static String getHostName();  //! get my own hostname

    static uint32_t getCalibreMinVal();
    static uint32_t getCalibreMaxVal();
    static double getCalibreFactor();
    static uint32_t getCurrentMiliVolts()
    {
      return AppPrefs::currentMiliVolts;
    }
    static float getCurrentPressureBar()
    {
      return AppPrefs::currentPressureBar;
    }
    //
    static void setCalibreMinVal( uint32_t );
    static void setCalibreMaxVal( uint32_t );
    static void setCalibreFactor( double );
    static void setCurrentMiliVolts( uint32_t );
    static void setCurrentPressureBar( float );
    static bool getWasChanged()
    {
      return AppPrefs::presureWasChanged;
    }
    static void resetWasChanged()
    {
      AppPrefs::presureWasChanged = false;
    }

    private:
    static bool getIfPrefsInit();        //! internal, is preferences initialized?
    static bool setIfPrefsInit( bool );  //! internal, set preferences initialized?
  };

}  // namespace prefs