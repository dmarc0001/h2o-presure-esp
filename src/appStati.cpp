#include <limits>
#include <cmath>
#include "appStati.hpp"
#include "statics.hpp"

namespace prefs
{
  //
  // constants for prefs save
  //
  constexpr const char *CHECKVAL{ "conf_init" };
  constexpr const char *INITVAL{ "wasInit" };

  constexpr const char *DEBUGSETTING{ "debugval" };
  constexpr const char *LOC_HOSTNAME{ "hostname" };
  constexpr const char *LOC_TIMEZONE{ "loc_tz" };
  constexpr const char *LOC_TIME_OFFSET{ "loc_tmz_offs" };
  constexpr const char *CAL_MINVAL{ "cal_min" };
  constexpr const char *CAL_MAXVAL{ "cal_max" };
  constexpr const char *CAL_FACTOR{ "cal_factor" };
  constexpr const char *MEASURE_TIMEDIFF{ "measure_diff" };
  constexpr const char *SIGNAL_LED_BRIGHTNESS{ "led_brightness" };

  //
  // init static variables
  //
  const char *AppStati::tag{ "AppStati" };
  bool AppStati::wasInit{ false };
  Preferences AppStati::lPref;
  uint32_t AppStati::calibreMinVal{ std::numeric_limits< uint32_t >::max() };
  uint32_t AppStati::calibreMaxVal{ std::numeric_limits< uint32_t >::max() };
  double AppStati::calibreFactor{ -10.0 };
  uint32_t AppStati::currentMiliVolts{ 0 };
  float AppStati::currentPressureBar{ 0.0F };
  volatile bool AppStati::presureWasChanged{ true };
  volatile bool AppStati::httpActive{ false };
  volatile bool AppStati::wasMeasure{ false };
  WlanState AppStati::wlanState{ WlanState::DISCONNECTED };
  size_t AppStati::fsTotalSpace{ 0 };
  size_t AppStati::fsUsedSpace{ 0 };

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
      AppStati::lPref.putString( LOC_TIMEZONE, "GMT" );
      AppStati::lPref.putLong( LOC_TIME_OFFSET, 0L );
      AppStati::lPref.putUInt( CAL_MINVAL, PRESSURE_MIN_MILIVOLT );
      AppStati::lPref.putUInt( CAL_MAXVAL, PRESSURE_MAX_MILIVOLT );
      AppStati::lPref.putDouble( CAL_FACTOR, PRESSURE_CALIBR_VALUE );
      AppStati::lPref.putUInt( MEASURE_TIMEDIFF, MEASURE_DIFF_TIME_S );
      AppStati::lPref.putUShort( SIGNAL_LED_BRIGHTNESS, LED_GLOBAL_BRIGHTNESS );
      Serial.println( "first-time-init preferences...DONE" );
      AppStati::setIfPrefsInit( true );
    }
    AppStati::wasInit = true;
  }

  /**
   * get the ground brightness from LED
   */
  uint8_t AppStati::getLedBrightness()
  {
    return static_cast< uint8_t >( AppStati::lPref.getUShort( SIGNAL_LED_BRIGHTNESS, LED_GLOBAL_BRIGHTNESS ) );
  }

  /**
   * set the ground brightness from LED
   */
  bool AppStati::setLedBrightness( uint8_t _val )
  {
    return ( AppStati::lPref.putUShort( SIGNAL_LED_BRIGHTNESS, static_cast< uint16_t >( _val ) ) > 0 );
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

  /**
   * get local timezone
   */
  String AppStati::getTimeZone()
  {
    return ( AppStati::lPref.getString( LOC_TIMEZONE, "GMT" ) );
  }

  /**
   * set locval timezone offset
   */
  bool AppStati::setTimezoneOffset( long _offset )
  {
    return ( AppStati::lPref.putLong( LOC_TIME_OFFSET, _offset ) > 0 );
  }

  /**
   * get local timezone offset
   */
  long AppStati::getTimezoneOffset()
  {
    return AppStati::lPref.getLong( LOC_TIME_OFFSET, 0L );
  }

  /**
   * set local timezone
   */
  bool AppStati::setTimeZone( const String &_lzone )
  {
    return ( AppStati::lPref.putString( LOC_TIMEZONE, _lzone.c_str() ) > 0 );
  }

  uint8_t AppStati::getLogLevel()
  {
#ifdef BUILD_DEBUG
    return ( AppStati::lPref.getUChar( DEBUGSETTING, DEBUG ) );
#else
    return ( AppStati::lPref.getUChar( DEBUGSETTING, INFO ) );
#endif
  }

  bool AppStati::setLogLevel( uint8_t _set )
  {
    return ( AppStati::lPref.putUChar( DEBUGSETTING, _set ) == 1 );
  }

  uint32_t AppStati::getMeasureInterval_s()
  {
    return AppStati::lPref.getUInt( MEASURE_TIMEDIFF, MEASURE_DIFF_TIME_S );
  }

  bool AppStati::setMeasureInterval_s( uint32_t _val )
  {
    return ( AppStati::lPref.putUInt( MEASURE_TIMEDIFF, _val ) > 0 );
  }

}  // namespace prefs