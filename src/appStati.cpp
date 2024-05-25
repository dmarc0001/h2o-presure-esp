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
  const char *AppStati::tag{ "AppStati" };
  bool AppStati::wasInit{ false };
  bool AppStati::spiffsInit{ false };
  Preferences AppStati::lPref;
  uint32_t AppStati::calibreMinVal{ std::numeric_limits< uint32_t >::max() };
  uint32_t AppStati::calibreMaxVal{ std::numeric_limits< uint32_t >::max() };
  double AppStati::calibreFactor{ -10.0 };
  uint32_t AppStati::currentMiliVolts{ 0 };
  float AppStati::currentPressureBar{ 0.0F };
  volatile bool AppStati::presureWasChanged{ true };

  void AppStati::init()
  {
    AppStati::lPref.begin( APPNAME, false );
    if ( AppStati::wasInit )
      return;
    if ( !AppStati::getIfPrefsInit() )
    {
      Serial.println( "first-time-init preferences..." );
      char hostname[ 32 ];
      uint16_t chip = static_cast< uint16_t >( ESP.getEfuseMac() >> 32 );
      snprintf( hostname, 32, "%s-%08X", DEFAULT_HOSTNAME, chip );
      String hn( &hostname[ 0 ] );
      AppStati::lPref.putString( LOC_HOSTNAME, hn );
      AppStati::lPref.putUInt( CAL_MINVAL, PRESSURE_MIN_MILIVOLT );
      AppStati::lPref.putUInt( CAL_MAXVAL, PRESSURE_MAX_MILIVOLT );
      AppStati::lPref.putDouble( CAL_FACTOR, PRESSURE_CALIBR_VALUE );
      AppStati::setIfPrefsInit( true );
      Serial.println( "first-time-init preferences...DONE" );
    }
    AppStati::wasInit = true;
  }

  /**
   * get the local hostname
   */
  String AppStati::getHostName()
  {
    return ( AppStati::lPref.getString( LOC_HOSTNAME, DEFAULT_HOSTNAME ) );
  }

  uint32_t AppStati::getCalibreMinVal()
  {
    if ( AppStati::calibreMinVal == std::numeric_limits< uint32_t >::max() )
    {
      AppStati::calibreMinVal = AppStati::lPref.getUInt( CAL_FACTOR, PRESSURE_MIN_MILIVOLT );
    }
    return AppStati::calibreMinVal;
  }

  uint32_t AppStati::getCalibreMaxVal()
  {
    if ( AppStati::calibreMaxVal == std::numeric_limits< uint32_t >::max() )
    {
      AppStati::calibreMaxVal = AppStati::lPref.getUInt( CAL_MAXVAL, PRESSURE_MAX_MILIVOLT );
    }
    return AppStati::calibreMaxVal;
  }

  double AppStati::getCalibreFactor()
  {
    if ( AppStati::calibreFactor < 0.0F )
    {
      AppStati::calibreFactor = AppStati::lPref.getDouble( CAL_FACTOR, PRESSURE_CALIBR_VALUE );
    }
    return AppStati::calibreFactor;
  }

  void AppStati::setCalibreMinVal( uint32_t _val )
  {
    AppStati::lPref.putUInt( CAL_MINVAL, _val );
    AppStati::calibreMinVal = _val;
  }

  void AppStati::setCalibreMaxVal( uint32_t _val )
  {
    AppStati::lPref.putUInt( CAL_MAXVAL, _val );
    AppStati::calibreMaxVal = _val;
  }

  void AppStati::setCalibreFactor( double _val )
  {
    AppStati::lPref.putDouble( CAL_FACTOR, _val );
    AppStati::calibreFactor = _val;
  }

  void AppStati::setCurrentMiliVolts( uint32_t _val )
  {
    if ( AppStati::currentMiliVolts != _val )
    {
      AppStati::currentMiliVolts = _val;
      // presureWasChanged = true;
    }
  }

  void AppStati::setCurrentPressureBar( float _val )
  {
    //
    // https://stackoverflow.com/questions/14369673/round-double-to-3-points-decimal
    //  number 60, thanks!
    //
    float val = std::round( _val / 0.01 ) * 0.01;
    if ( AppStati::currentPressureBar != val )
    {
      AppStati::currentPressureBar = val;
      presureWasChanged = true;
    }
  }

  /**
   * Check if preferences was initialized once
   */
  bool AppStati::getIfPrefsInit()
  {
    String defaultVal( "-" );
    String correctVal( INITVAL );
    return ( correctVal == AppStati::lPref.getString( CHECKVAL, defaultVal ) );
  }

  /**
   * set if prefs was initialized
   */
  bool AppStati::setIfPrefsInit( bool _set )
  {
    if ( _set )
    {
      // if set => set the correct value
      return ( AppStati::lPref.putString( CHECKVAL, INITVAL ) > 0 );
    }
    // else remove the key, set the properties not valid
    return AppStati::lPref.remove( CHECKVAL );
  }

}  // namespace prefs