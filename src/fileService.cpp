#include <regex>
#include <cstdlib>
#include <TimeLib.h>
#include "statics.hpp"
#include "fileService.hpp"
#include "appStati.hpp"

namespace measure_h2o
{
  const char *FileService::tag{ "FileService" };
  bool FileService::wasInit{ false };
  SemaphoreHandle_t FileService::measureFileSem{ nullptr };
  presure_data_set_t FileService::dataset{};
  TaskHandle_t FileService::taskHandle{ nullptr };

  void FileService::init()
  {
    //
    // initialize file handling
    //
    elog.log( INFO, "%s: init file handling...", FileService::tag );
    if ( FileService::wasInit )
      return;
    //
    if ( !prefs::AppStati::getIsSpiffsInit() )
    {
      elog.log( DEBUG, "%s: init filesystem...", FileService::tag );
      if ( !SPIFFS.begin( false, prefs::WEB_PATH, 8, prefs::WEB_PARTITION_LABEL ) )
      {
        // TODO: Errormessage
        elog.log( INFO, "%s: init failed, FORMAT filesystem...", FileService::tag );
        if ( !SPIFFS.format() )
        {
          // there is an error BAD!
          // TODO: Errormessage
          elog.log( ERROR, "%s: An Error has occurred while mounting SPIFFS!", FileService::tag );
          delay( 5000 );
        }
        else
        {
          elog.log( INFO, "%s: FORMAT filesystem successful...", FileService::tag );
          // is okay
          prefs::AppStati::setIsSpiffsInit( true );
        }
        ESP.restart();
      }
      else
      {
        // is okay
        prefs::AppStati::setIsSpiffsInit( true );
        FileService::start();
        elog.log( DEBUG, "%s: init filesystem...OK", FileService::tag );
      }
    }
    // init semaphore for access to datafiles
    vSemaphoreCreateBinary( measureFileSem );
  }

  void FileService::start()
  {
    elog.log( INFO, "%s: Task start...", FileService::tag );

    if ( FileService::taskHandle )
    {
      vTaskDelete( FileService::taskHandle );
      FileService::taskHandle = nullptr;
    }
    else
    {
      xTaskCreate( FileService::sTask, "m-task", configMINIMAL_STACK_SIZE * 4, nullptr, tskIDLE_PRIORITY, &FileService::taskHandle );
    }
  }

  void FileService::sTask( void * )
  {
    static uint64_t nextTimeToCheck = prefs::FILE_TASK_DELAY_YS;
    static uint64_t nextTimeToFSCheck = prefs::FILE_TASK_CHECK_DELAY_YS;

    while ( true )
    {
      uint64_t nowTime = esp_timer_get_time();

      if ( nowTime > nextTimeToFSCheck )
      {
        FileService::checkFileSys();
        nextTimeToFSCheck = nowTime + prefs::FILE_TASK_CHECK_DELAY_YS;
      }

      if ( nowTime > nextTimeToCheck )
      {
        if ( !FileService::dataset.empty() )
        {
          //
          // there are datas to store
          //
          FileService::saveDatasets();
        }
        // else
        // {
        //   elog.log( DEBUG, "%s: there are NO data for store...", FileService::tag );
        // }
        nextTimeToCheck = nowTime + prefs::FILE_TASK_DELAY_YS;
      }
    }
  }

  int FileService::saveDatasets()
  {
    int savedCount{ 0 };

    elog.log( DEBUG, "%s: there are <%d> datasets for store...", FileService::tag, FileService::dataset.size() );
    if ( xSemaphoreTake( FileService::measureFileSem, pdMS_TO_TICKS( 6000 ) ) == pdTRUE )
    {
      char buffer[ 28 ];
      snprintf( buffer, 28, prefs::DAYLY_FILE_NAME, year(), month(), day() );
      String fileName( prefs::DATA_PATH );
      fileName += String( "/" ) + String( buffer );

      // open/create File mode append
      auto fh = SPIFFS.open( fileName, "a", true );
      if ( fh )
      {
        elog.log( DEBUG, "%s: datafile <%s> opened...", FileService::tag, fileName.c_str() );
        while ( FileService::dataset.size() > 0 )
        {
          //
          // while all datasets are computed
          // make a copy from first
          //
          presure_data_t elem = FileService::dataset.front();
          // delete it from vector
          FileService::dataset.erase( FileService::dataset.begin() );
          //
          fh.print( elem.timestamp );
          fh.print( "," );
          snprintf( buffer, 6, "%02.2f\0", elem.pressureBar );
          fh.print( "," );
          snprintf( buffer, 8, "%06d\0", elem.miliVolts );
          fh.print( buffer );
          fh.print( "\n" );
          fh.flush();
          ++savedCount;
          //
        }
        elog.log( DEBUG, "%s: datafile <%s> <%d> lines written, close...", FileService::tag, fileName.c_str(), savedCount );
        fh.close();
      }
      else
      {
        while ( !FileService::dataset.empty() )
        {
          FileService::dataset.erase( FileService::dataset.begin() );
        }
        elog.log( ERROR, "%s: datafile <%s> can't open, data lost!", FileService::tag, fileName.c_str() );
      }
    }
    // We have finished accessing the shared resource.  Release the
    // semaphore.
    xSemaphoreGive( FileService::measureFileSem );
    return savedCount;
  }

  int FileService::checkFileSys()
  {
    File root = SPIFFS.open( prefs::DATA_PATH );
    std::regex reg( prefs::DAYLY_FILE_PATTERN );
    std::smatch match;
    std::string fname( root.getNextFileName().c_str() );
    std::vector< String > fileList;

    //
    // find my files in filenames
    //
    while ( !fname.empty() )
    {
      // elog.log( DEBUG, "%s: === found file <%s>", FileService::tag, fname.c_str() );
      //
      // is the Filename like ma pattern
      //
      if ( std::regex_search( fname, match, reg ) )
      {
        //
        // filename matches
        // store in Vector
        //
        String matchedFileName( fname.c_str() );
        // elog.log( DEBUG, "%s: +++ found file who match <%s>", FileService::tag, fname.c_str() );
        fileList.push_back( matchedFileName );
      }
      fname = root.getNextFileName().c_str();
      delay( 1 );
    }
    root.close();
    //
    // find outdated files
    //
    int prefix = strlen( prefs::DATA_PATH ) + 1;
    tmElements_t currentTime;
    currentTime.Hour = hour();
    currentTime.Minute = minute();
    currentTime.Second = second();
    currentTime.Day = day();
    currentTime.Month = month();
    currentTime.Year = year() - 1970;  // because Year is offset from 1970
    //
    // all filenames
    //
    for ( String fileName : fileList )
    {
      //
      // make from filename a dateTime
      //
      String nameShort = fileName.substring( prefix, prefix + 10 );
      // elog.log( DEBUG, "%s: check file <%s>", FileService::tag, nameShort.c_str() );
      time_t fYear = nameShort.substring( 0, 4 ).toInt();
      time_t fMonth = nameShort.substring( 5, 7 ).toInt();
      time_t fDay = nameShort.substring( 8, 10 ).toInt();
      // elog.log( DEBUG, "%s: check file <%s> y: <%04d> m: <%02d> d: <%02d>", FileService::tag, nameShort.c_str(), fYear, fMonth, fDay
      // );
      time_t nowTime = now();
      tmElements_t fileTime;
      fileTime.Hour = 0;
      fileTime.Minute = 0;
      fileTime.Second = 0;
      fileTime.Day = fDay;
      fileTime.Month = fMonth;
      fileTime.Year = fYear - 1970;  // because Year is offset from 1970
      //
      // change to secounds
      //
      time_t fileTimeStamp = makeTime( fileTime );
      time_t currentTimeStamp = makeTime( currentTime );
      //
      // is the file older than max age?
      // TODO: TEST it if NTP works
      if ( std::abs( currentTimeStamp - fileTimeStamp ) > prefs::MAX_DATA_FILE_AGE_SEC )
      {
        elog.log( INFO, "%s: file <%s> is too old, delete it!", FileService::tag, nameShort.c_str() );
        SPIFFS.remove( fileName );
      }
      // else
      //   elog.log( DEBUG, "%s: file <%s> in range of age, do nothing.", FileService::tag, nameShort.c_str() );
      delay( 1 );
    }
    return 0;
  }
}  // namespace measure_h2o
