#include <esp_spiffs.h>
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
  String FileService::todayFileName;
  int FileService::todayDay{ -1 };

  /**
   * init this object (single)
   */
  void FileService::init()
  {
    //
    // initialize file handling
    //
    elog.log( INFO, "%s: init file handling...", FileService::tag );
    if ( FileService::wasInit )
      return;
    //
    // init semaphore for access to datafiles
    //
    vSemaphoreCreateBinary( measureFileSem );
    FileService::start();
  }

  /**
   * internal: start service task
   */
  void FileService::start()
  {
    elog.log( INFO, "%s: Task start...", FileService::tag );
    delay( 10 );

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

  /**
   * service task
   */
  void FileService::sTask( void * )
  {
    static uint64_t nextTimeToCheck = prefs::FILE_TASK_DELAY_YS;
    static uint64_t nextSystemFsCheck = esp_timer_get_time() + prefs::FILE_SYSTEM_SIZE_CHECK_YS;
    static uint64_t nextTimeToFSCheck = esp_timer_get_time() + prefs::FILE_TASK_CHECK_DELAY_YS;

    while ( true )
    {
      uint64_t nowTime = esp_timer_get_time();

      if ( nowTime > nextSystemFsCheck )
      {
        //
        // check if filesystem free size too small
        //
        FileService::computeFilesystemCheck();
        //
        // next test
        //
        nextSystemFsCheck = nowTime + prefs::FILE_SYSTEM_SIZE_CHECK_YS;
      }

      if ( nowTime > nextTimeToFSCheck )
      {
        //
        // check if filenames have to switch
        //
        FileService::checkFileSys();
        nextTimeToFSCheck = nowTime + prefs::FILE_TASK_CHECK_DELAY_YS;
      }

      if ( nowTime > nextTimeToCheck )
      {
        //
        // first, test if force filessystem check initiated
        //
        if ( prefs::AppStati::getForceFilesystemCheck() )
        {
          prefs::AppStati::setForceFilesystemCheck( false );
          FileService::computeFilesystemCheck();
        }
        //
        // check if data have to save
        //
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

  /**
   * do the filesystemchecks
   */
  int FileService::computeFilesystemCheck()
  {
    elog.log( INFO, "%s: compute filesestem check...", FileService::tag );
    //
    // first the "normal way"
    //
    FileService::checkFileSys();
    //
    // than a bit more aggressive, if needed
    //
    if ( FileService::checkFileSysSizes() != 0 )
    {
      //
      // free flash memory....
      //
      // first: delete other than current
      // TODO: WARNING
      FileService::deleteOtherThanCurrent();
      //
      // check again
      //
      if ( FileService::checkFileSysSizes() != 0 )
      {
        // ALERT!
        // remove current file
        // TODO: ERROR MESSAGE
        elog.log( ERROR, "%s: delete current file(s) while no space left for measures...", FileService::tag );
        FileService::deleteTodayFile();
      }
    }
    return 0;
  }

  /**
   * delete files there not the current file
   */
  int FileService::deleteOtherThanCurrent()
  {
    //
    // find files in path prefs::DATA_PATH
    //
    File root = SPIFFS.open( String( prefs::DATA_PATH ).substring( 0, strlen( prefs::DATA_PATH ) - 1 ) );
    std::regex reg( prefs::DAYLY_FILE_PATTERN );
    std::smatch match;
    std::string fname( root.getNextFileName().c_str() );
    std::vector< String > fileList;

    elog.log( INFO, "%s: delete other than current file(s)...", FileService::tag );
    //
    // find my files in filenames
    //
    while ( !fname.empty() )
    {
      elog.log( DEBUG, "%s: === found file <%s>", FileService::tag, fname.c_str() );
      //
      // is the Filename like my pattern
      //
      if ( std::regex_search( fname, match, reg ) )
      {
        //
        // filename matches
        // store in Vector
        //
        String matchedFileName( fname.c_str() );
        elog.log( DEBUG, "%s: +++ found file who match <%s> as delete candidate...", FileService::tag, fname.c_str() );
        fileList.push_back( matchedFileName );
      }
      fname = root.getNextFileName().c_str();
      delay( 10 );
    }
    root.close();
    if ( fileList.empty() )
      return 0;
    //
    // okay check files against current
    //
    int prefix = strlen( prefs::DATA_PATH );
    //
    // all filenames
    //
    for ( String fileName : fileList )
    {
      //
      // make from filename a dateTime
      //
      String nameShort = fileName.substring( prefix, prefix + 10 );
      String currentShort = FileService::todayFileName.substring( prefix, prefix + 10 );
      if ( nameShort.compareTo( currentShort ) != 0 )
      {
        elog.log( INFO, "%s: file <%s> is too old, delete it!", FileService::tag, nameShort.c_str() );
        SPIFFS.remove( fileName );
      }
      delay( 10 );
    }
    return 0;
  }

  /**
   * check filesystem sizes and free space
   */
  int FileService::checkFileSysSizes()
  {
    size_t flash_total;
    size_t flash_used;
    size_t flash_free;

    elog.log( DEBUG, "%s: check filesystem size", FileService::tag );
    esp_err_t errorcode = esp_spiffs_info( prefs::WEB_PARTITION_LABEL, &flash_total, &flash_used );
    if ( errorcode == ESP_OK )
    {
      prefs::AppStati::setFsTotalSpace( flash_total );
      prefs::AppStati::setFsUsedSpace( flash_used );
      flash_free = flash_total - flash_used;
      elog.log( DEBUG, "%s: SPIFFS total %07d, used %07d, free %07d, min-free: %07d", FileService::tag, flash_total, flash_used,
                flash_free, prefs::MIN_FILE_SYSTEM_FREE_SIZE );
      //
      // TODO: esp_err_t esp_spiffs_gc(const char *partition_label, size_t size_to_gc)
      //
      if ( prefs::MIN_FILE_SYSTEM_FREE_SIZE > flash_free )
      {
        elog.log( WARNING, "%s: free memory too low, action needed", FileService::tag );
        return -1;
      }
      return 0;
    }
    else
    {
      elog.log( ERROR, "%s: can't check SPIFFS memory!", FileService::tag );
    }
    return -1;
  }

  /**
   * delete todays file (e.g. set measure interval)
   */
  bool FileService::deleteTodayFile()
  {
    if ( xSemaphoreTake( FileService::measureFileSem, pdMS_TO_TICKS( 6000 ) ) == pdTRUE )
    {
      String fName = FileService::getTodayFileName();
      if ( fName )
      {
        SPIFFS.remove( fName );
        xSemaphoreGive( FileService::measureFileSem );
        return true;
      }
      xSemaphoreGive( FileService::measureFileSem );
      return false;
    }
    return false;
  }

  /**
   * save dataset(s) from queue to measure file
   */
  int FileService::saveDatasets()
  {
    int savedCount{ 0 };

    elog.log( DEBUG, "%s: there are <%d> datasets for store...", FileService::tag, FileService::dataset.size() );
    if ( xSemaphoreTake( FileService::measureFileSem, pdMS_TO_TICKS( 6000 ) ) == pdTRUE )
    {
      char buffer[ 28 ];
      FileService::getTodayFileName();
      // open/create File mode append
      auto fh = SPIFFS.open( FileService::todayFileName, "a", true );
      if ( fh )
      {
        elog.log( DEBUG, "%s: datafile <%s> opened...", FileService::tag, FileService::todayFileName.c_str() );
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
          fh.print( buffer );
          fh.print( "," );
          snprintf( buffer, 8, "%06d\0", elem.miliVolts );
          fh.print( buffer );
          fh.print( "\n" );
          fh.flush();
          ++savedCount;
        }
        elog.log( DEBUG, "%s: datafile <%s> <%d> lines written...", FileService::tag, FileService::todayFileName.c_str(), savedCount );
        fh.close();
      }
      else
      {
        while ( !FileService::dataset.empty() )
        {
          FileService::dataset.erase( FileService::dataset.begin() );
        }
        elog.log( ERROR, "%s: datafile <%s> can't open, data lost!", FileService::tag, FileService::todayFileName.c_str() );
      }
    }
    // We have finished accessing the shared resource.  Release the
    // semaphore.
    xSemaphoreGive( FileService::measureFileSem );
    return savedCount;
  }

  /**
   * chcek filesystem if i have to care data
   */
  int FileService::checkFileSys()
  {
    //
    FileService::checkFileSysSizes();

    //
    // find files in path prefs::DATA_PATH
    //
    File root = SPIFFS.open( String( prefs::DATA_PATH ).substring( 0, strlen( prefs::DATA_PATH ) - 1 ) );
    std::regex reg( prefs::DAYLY_FILE_PATTERN );
    std::smatch match;
    std::string fname( root.getNextFileName().c_str() );
    std::vector< String > fileList;

    elog.log( INFO, "%s: filesystem check, search older files...", FileService::tag );
    //
    // find my files in filenames
    //
    while ( !fname.empty() )
    {
      elog.log( DEBUG, "%s: === found file <%s>", FileService::tag, fname.c_str() );
      //
      // is the Filename like my pattern
      //
      if ( std::regex_search( fname, match, reg ) )
      {
        //
        // filename matches
        // store in Vector
        //
        String matchedFileName( fname.c_str() );
        elog.log( DEBUG, "%s: +++ found file who match <%s>", FileService::tag, fname.c_str() );
        fileList.push_back( matchedFileName );
      }
      fname = root.getNextFileName().c_str();
      delay( 10 );
    }
    root.close();
    if ( fileList.empty() )
      return 0;
    //
    // find outdated files
    //
    int prefix = strlen( prefs::DATA_PATH );
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
      delay( 10 );
    }
    return 0;
  }

  /**
   * get the filename for today
   */
  String &FileService::getTodayFileName()
  {
    if ( FileService::todayDay != day() )
    {
      char buffer[ 28 ];
      FileService::todayDay = day();
      snprintf( buffer, 28, prefs::DAYLY_FILE_NAME, year(), month(), day() );
      String fileName( prefs::DATA_PATH );
      fileName += String( buffer );
      FileService::todayFileName = fileName;
    }
    return todayFileName;
  }

}  // namespace measure_h2o

// 2024-10-06 07:05:18 945 [GLO] [DEBUG] : PrSensor: pressure measure...
// 2024-10-06 07:05:27 622 [GLO] [DEBUG] : FileService: there are <1> datasets for store...
// 2024-10-06 07:05:27 858 [GLO] [DEBUG] : FileService: datafile </data/2024-10-06-pressure.csv> opened...
// 2024-10-06 07:05:27 859 [GLO] [DEBUG] : FileService: datafile </data/2024-10-06-pressure.csv> <1> lines written...
// 2024-10-06 07:09:19 276 [GLO] [DEBUG] : PrSensor: pressure measure...
// 2024-10-06 07:09:27 742 [GLO] [DEBUG] : FileService: there are <1> datasets for store...
// 2024-10-06 07:09:28 075 [GLO] [DEBUG] : FileService: datafile </data/2024-10-06-pressure.csv> opened...
// 2024-10-06 07:09:28 075 [GLO] [DEBUG] : FileService: datafile </data/2024-10-06-pressure.csv> <1> lines written...
// 2024-10-06 07:10:17 365 [GLO] [INFO ] : WifiConfig: device disconnected from accesspoint...
// 2024-10-06 07:10:17 831 [GLO] [WARN ] : main: ip connectivity lost, stop webserver.
// 2024-10-06 07:10:18 376 [GLO] [INFO ] : WifiConfig: device disconnected from accesspoint...
// 2024-10-06 07:56:42 000 [GLO] [DEBUG] : main: logger time correction...
// 2024-10-06 07:56:42 000 [GLO] [DEBUG] : main: gotten system time!
// 2024-10-06 07:56:42 000 [GLO] [DEBUG] : main: logger time correction...OK
// 2024-10-06 07:56:44 091 [GLO] [DEBUG] : FileService: get file infos...
// 2024-10-06 07:56:44 091 [GLO] [DEBUG] : FileService: total SPIFFS space: 1216346
// 2024-10-06 07:56:44 091 [GLO] [DEBUG] : FileService: used SPIFFS space: 0037399
// 2024-10-06 07:56:46 000 [GLO] [INFO ] : FileService: filesystem check, search older files...
// 2024-10-06 07:56:46 000 [GLO] [DEBUG] : FileService: === found file </data/2024-10-03-pressure.csv>
// 2024-10-06 07:56:46 017 [GLO] [DEBUG] : FileService: === found file </data/2024-10-04-pressure.csv>
// 2024-10-06 07:56:46 035 [GLO] [DEBUG] : FileService: === found file </data/2024-10-05-pressure.csv>
// 2024-10-06 07:56:46 048 [GLO] [DEBUG] : FileService: === found file </data/2024-10-06-pressure.csv>
