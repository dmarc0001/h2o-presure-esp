#include <limits>
#include "appStati.hpp"
#include "statics.hpp"

namespace prefs
{
  //
  // constants for prefs save
  //
  constexpr const char *CHECKVAL{ "confInit" };
  constexpr const char *INITVAL{ "wasInit" };

  constexpr const char *LOC_HOSTNAME{ "hostname" };
  constexpr const char *CAL_MINVAL{ "cal_min" };
  constexpr const char *CAL_MAXVAL{ "cal_max" };
  constexpr const char *CAL_FACTOR{ "cal_factor" };

  //
  // init static variables
  //
  const char *AppPrefs::tag{ "Prefs" };
  bool AppPrefs::wasInit{ false };
  Preferences AppPrefs::lPref;
  uint32_t AppPrefs::calibreMinVal{ std::numeric_limits< uint32_t >::max() };
  uint32_t AppPrefs::calibreMaxVal{ std::numeric_limits< uint32_t >::max() };
  double AppPrefs::calibreFactor{ -10.0 };
  uint32_t AppPrefs::currentMiliVolts{ 0 };
  float AppPrefs::currentPressureBar{ 0.0F };
  volatile bool AppPrefs::presureWasChanged{ true };

  void AppPrefs::init()
  {
    AppPrefs::lPref.begin( APPNAME, false );
    if ( AppPrefs::wasInit )
      return;
    if ( !AppPrefs::getIfPrefsInit() )
    {
      Serial.println( "first-time-init preferences..." );
      char hostname[ 32 ];
      uint16_t chip = static_cast< uint16_t >( ESP.getEfuseMac() >> 32 );
      snprintf( hostname, 32, "%s-%08X", DEFAULT_HOSTNAME, chip );
      String hn( &hostname[ 0 ] );
      AppPrefs::lPref.putString( LOC_HOSTNAME, hn );
      AppPrefs::lPref.putUInt( CAL_MINVAL, PRESSURE_MIN_MILIVOLT );
      AppPrefs::lPref.putUInt( CAL_MAXVAL, PRESSURE_MAX_MILIVOLT );
      AppPrefs::lPref.putDouble( CAL_FACTOR, PRESSURE_CALIBR_VALUE );
      AppPrefs::setIfPrefsInit( true );
      Serial.println( "first-time-init preferences...DONE" );
    }
    AppPrefs::wasInit = true;
  }

  /**
   * get the local hostname
   */
  String AppPrefs::getHostName()
  {
    return ( AppPrefs::lPref.getString( LOC_HOSTNAME, DEFAULT_HOSTNAME ) );
  }

  uint32_t AppPrefs::getCalibreMinVal()
  {
    if ( AppPrefs::calibreMinVal == std::numeric_limits< uint32_t >::max() )
    {
      AppPrefs::calibreMinVal = AppPrefs::lPref.getUInt( CAL_FACTOR, PRESSURE_MIN_MILIVOLT );
    }
    return AppPrefs::calibreMinVal;
  }

  uint32_t AppPrefs::getCalibreMaxVal()
  {
    if ( AppPrefs::calibreMaxVal == std::numeric_limits< uint32_t >::max() )
    {
      AppPrefs::calibreMaxVal = AppPrefs::lPref.getUInt( CAL_MAXVAL, PRESSURE_MAX_MILIVOLT );
    }
    return AppPrefs::calibreMaxVal;
  }

  double AppPrefs::getCalibreFactor()
  {
    if ( AppPrefs::calibreFactor < 0.0F )
    {
      AppPrefs::calibreFactor = AppPrefs::lPref.getDouble( CAL_FACTOR, PRESSURE_CALIBR_VALUE );
    }
    return AppPrefs::calibreFactor;
  }

  void AppPrefs::setCalibreMinVal( uint32_t _val )
  {
    AppPrefs::lPref.putUInt( CAL_MINVAL, _val );
    AppPrefs::calibreMinVal = _val;
  }

  void AppPrefs::setCalibreMaxVal( uint32_t _val )
  {
    AppPrefs::lPref.putUInt( CAL_MAXVAL, _val );
    AppPrefs::calibreMaxVal = _val;
  }

  void AppPrefs::setCalibreFactor( double _val )
  {
    AppPrefs::lPref.putDouble( CAL_FACTOR, _val );
    AppPrefs::calibreFactor = _val;
  }

  void AppPrefs::setCurrentMiliVolts( uint32_t _val )
  {
    if ( AppPrefs::currentMiliVolts != _val )
    {
      AppPrefs::currentMiliVolts = _val;
      // presureWasChanged = true;
    }
  }

  void AppPrefs::setCurrentPressureBar( float _val )
  {
    //
    // https://stackoverflow.com/questions/14369673/round-double-to-3-points-decimal
    //  number 60, thanks!
    //
    float val = std::round( _val / 0.01 ) * 0.01;
    if ( AppPrefs::currentPressureBar != val )
    {
      AppPrefs::currentPressureBar = val;
      presureWasChanged = true;
    }
  }

  /**
   * Check if preferences was initialized once
   */
  bool AppPrefs::getIfPrefsInit()
  {
    String defaultVal( "-" );
    String correctVal( INITVAL );
    return ( correctVal == AppPrefs::lPref.getString( CHECKVAL, defaultVal ) );
  }

  /**
   * set if prefs was initialized
   */
  bool AppPrefs::setIfPrefsInit( bool _set )
  {
    if ( _set )
    {
      // if set => set the correct value
      return ( AppPrefs::lPref.putString( CHECKVAL, INITVAL ) > 0 );
    }
    // else remove the key, set the properties not valid
    return AppPrefs::lPref.remove( CHECKVAL );
  }

}  // namespace prefs