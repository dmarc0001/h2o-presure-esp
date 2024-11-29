#include <memory>
#include <esp_chip_info.h>
#include <esp_spiffs.h>
#include <cstdlib>
#include "webServer.hpp"
#include "statics.hpp"
#include "appPrefs.hpp"
#include "appStati.hpp"
#include "fileService.hpp"

namespace measure_h2o
{
  const char *APIWebServer::tag{ "apiServer" };
  // instantiate a webserver
  AsyncWebServer APIWebServer::server( 80 );

  //
  // small html pages in GERNAM language
  //
  const char intervall_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html lang="de-DE">
    <head>
    <title>Messintervall</title>
    <meta name="description" content="water pressure monitor" />
    <meta name="keywords" content="save presure values" />
    <meta name="author" content="Dirk Marciniak" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="presure.css">
    </head>
    <body>
    <h1>Intervall der Messungen</h1
    <h3>%MEASURE_INTERVAL% Sekunden</h3>
    <br />
    <br />
    <div class="footnote">
    (c) Dirk Marciniak
    </div>
    </body>
  </html>
  )rawliteral";

  const char version_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html lang="de-DE">
    <head>
    <title>Server Version</title>
    <meta name="description" content="water pressure monitor" />
    <meta name="keywords" content="save presure values" />
    <meta name="author" content="Dirk Marciniak" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="presure.css">
    </head>
    <body>
    <h1>Interne Versionsnummer</h1>
    <h3>Version: %APP_VERSION%</h3>
    <br />
    <br />
    <div class="footnote">
    (c) Dirk Marciniak
    </div>
    </body>
  </html>
  )rawliteral";

  const char info_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html lang="de-DE">
    <head>
    <title>Server Information</title>
    <meta name="description" content="water pressure monitor" />
    <meta name="keywords" content="save presure values" />
    <meta name="author" content="Dirk Marciniak" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="presure.css">
    </head>
    <body>
    <h1>Server Informationen</h1>
    <h3>IDF Version: %IDF_VERSION%</h3>
    <h3>ESP Model: %ESP_MODEL%</h3>
    <h3>CORES: %ESP_CORES%</h3>
    <br />
    <br />
    <div class="footnote">
    (c) Dirk Marciniak
    </div>
    </body>
  </html>
  )rawliteral";

  /**
   * ini a few things?
   */
  void APIWebServer::init()
  {
    //
    // maybe a few things to init?
    //
    prefs::AppStati::init();
  }

  /**
   * start the webserver service
   */
  void APIWebServer::start()
  {
    elog.log( INFO, "%s: start webserver...", APIWebServer::tag );
    // Cache responses for 1 minutes (60 seconds)
    // fileroot in prefs::WEB_PATH
    APIWebServer::server.serveStatic( "/", SPIFFS, prefs::WEB_PATH ).setCacheControl( "max-age=60" );
    //
    // response filters
    //
    APIWebServer::server.on( "^/$", HTTP_GET, APIWebServer::onIndex );
    APIWebServer::server.on( "^/index\\.html$", HTTP_GET, APIWebServer::onIndex );
    APIWebServer::server.on( "^/metrics$", HTTP_GET, APIWebServer::onGetMetrics );
    APIWebServer::server.on( "^\\/api\\/v1\\/set-(.*)\?(.*)$", HTTP_GET, APIWebServer::onApiV1Set );
    APIWebServer::server.on( "^\\/api\\/v1\\/(.*)$", HTTP_GET, APIWebServer::onApiV1 );
    APIWebServer::server.on( "^\\/.*$", HTTP_GET, APIWebServer::onFilesReq );
    APIWebServer::server.onNotFound( APIWebServer::onNotFound );
    APIWebServer::server.begin();
    elog.log( DEBUG, "%s: start webserver...OK", APIWebServer::tag );
  }

  /**
   * stop the webserver service comlete
   */
  void APIWebServer::stop()
  {
    APIWebServer::server.reset();
    APIWebServer::server.end();
  }

  /**
   * response for index request
   */
  void APIWebServer::onIndex( AsyncWebServerRequest *request )
  {
    String file( "/www/index.html" );
    prefs::AppStati::httpActive = true;
    APIWebServer::deliverFileToHttpd( file, request );
  }

  /**
   * response for request a file (for a directory my fail)
   */
  void APIWebServer::onFilesReq( AsyncWebServerRequest *request )
  {
    prefs::AppStati::httpActive = true;
    String file( request->url() );
    APIWebServer::deliverFileToHttpd( file, request );
  }

  void APIWebServer::onGetMetrics( AsyncWebServerRequest *request )
  {
    char buffer[ 12 ];
    size_t flash_total;
    size_t flash_used;
    size_t flash_free;

    elog.log( DEBUG, "%s: access metrics...", APIWebServer::tag );
    prefs::AppStati::httpActive = true;
    //
    // per LINE:
    // metric_name [
    //   "{" label_name "=" `"` label_value `"` { "," label_name "=" `"` label_value `"` } [ "," ] "}"
    // ] value [ timestamp ]
    //
    //
    // https://prometheus.io/docs/instrumenting/exposition_formats/
    //

    // say prometheus that values are conters
    String msg( "# TYPE pressure counter\n" );
    // print last measured millivolts
    snprintf( buffer, 8, "%04d\0", prefs::AppStati::getCurrentMiliVolts() );
    msg += String( "pressure_measured_millivolts {meaning=\"millivolts\"} " ) + String( buffer ) + String( "\n" );
    // print last measured water pressure
    snprintf( buffer, 6, "%02.2f\0", prefs::AppStati::getCurrentPressureBar() );
    msg += String( "pressure_measured_pressure_value {meaning=\"water pressure\"} " ) + String( buffer ) + String( "\n" );
    // check flash memory
    esp_err_t errorcode = esp_spiffs_info( prefs::WEB_PARTITION_LABEL, &flash_total, &flash_used );
    if ( errorcode == ESP_OK )
    {
      prefs::AppStati::setFsTotalSpace( flash_total );
      prefs::AppStati::setFsUsedSpace( flash_used );
      flash_free = flash_total - flash_used;
    }
    // print total flash memory
    snprintf( buffer, 11, "%08d\0", prefs::AppStati::getFsTotalSpace() );
    msg += String( "pressure_total_flash {meaning=\"total space on flash\"} " ) + String( buffer ) + String( "\n" );
    // print used flash memory
    snprintf( buffer, 11, "%08d\0", prefs::AppStati::getFsUsedSpace() );
    msg += String( "pressure_used_flash {meaning=\"used space on flash\"} " ) + String( buffer ) + String( "\n" );
    // print freeram
    snprintf( buffer, 11, "%08d\0", ESP.getFreeHeap() );
    msg += String( "pressure_free_ram {meaning=\"free ram on esp32h\"} " ) + String( buffer ) + String( "\n" );
    // send to client
    request->send( 200, "text/plain", msg );
    return;
    // String msg = "ERROR api call v1 for <" + parameter + ">";
    // APIWebServer::onServerError( request, 303, msg );
  }

  /**
   * response for an api request, Version 1
   */
  void APIWebServer::onApiV1( AsyncWebServerRequest *request )
  {
    prefs::AppStati::httpActive = true;
    String parameter = request->pathArg( 0 );
    elog.log( DEBUG, "%s: api version 1 call <%s>", APIWebServer::tag, parameter );
    if ( parameter.equals( "today" ) )
    {
      APIWebServer::apiGetTodayData( request );
    }
    else if ( parameter.equals( "data" ) )
    {
      APIWebServer::apiGetRestDataFileFrom( request );
    }
    else if ( parameter.equals( "interval" ) )
    {
      APIWebServer::apiGetRestInterval( request );
    }
    else if ( parameter.equals( "fsstat" ) )
    {
      APIWebServer::apiGetRestFilesystemStatus( request );
    }
    else if ( parameter.equals( "led" ) )
    {
      APIWebServer::apiGetRestLedBrightness( request );
    }
    else if ( parameter.equals( "flash" ) )
    {
      APIWebServer::apiGetRestFlashAmount( request );
    }
    else
    {
      String msg = "ERROR api call v1 for <" + parameter + ">";
      APIWebServer::onServerError( request, 303, msg );
    }
  }

  /**
   * compute set commands via API
   */
  void APIWebServer::onApiV1Set( AsyncWebServerRequest *request )
  {
    prefs::AppStati::httpActive = true;
    String verb = request->pathArg( 0 );
    String server, port;

    elog.log( DEBUG, "%s: api version 1 call set-%s", APIWebServer::tag, verb );
    //
    // timezone set?
    //
    if ( verb.equals( "timezone" ) )
    {
      // timezone parameter find
      if ( request->hasParam( "timezone" ) )
      {
        String timezone = request->getParam( "timezone" )->value();
        elog.log( DEBUG, "%s: set-timezone, param: %s", APIWebServer::tag, timezone.c_str() );
        if ( prefs::AppStati::setTimeZone( timezone ) )
        {
          request->send( 200, "text/plain",
                         "OK api call v1 for <set-" + verb + "> = <" + timezone + "> BUT: not functional on this chip!" );
          setenv( "TZ", timezone.c_str(), 1 );
          tzset();
          // delay( 500 );
          // ESP.restart();
        }
        else
        {
          String msg = "ERROR api call v1 for <set-" + verb + "> = <" + timezone + ">";
          APIWebServer::onServerError( request, 303, msg );
        }
        return;
      }
      // timezone offset parameter find
      else if ( request->hasParam( "timezone-offset" ) )
      {
        String timezone = request->getParam( "timezone-offset" )->value();
        elog.log( DEBUG, "%s: set-%s, param: %s", APIWebServer::tag, verb.c_str(), timezone.c_str() );
        if ( prefs::AppStati::setTimezoneOffset( timezone.toInt() ) )
        {
          request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + "> = <" + timezone + ">" );
          yield();
          sleep( 1 );
          ESP.restart();
        }
        else
        {
          String msg = "ERROR api call v1 for <set-" + verb + "> = <" + timezone + ">";
          APIWebServer::onServerError( request, 303, msg );
        }
        return;
      }
      else
      {
        String msg = "api call v1 for <set-" + verb + "> param not found!";
        APIWebServer::onServerError( request, 303, msg );
        return;
      }
    }
    else if ( verb.equals( "loglevel" ) )
    {
      // loglevel parameter find
      if ( request->hasParam( "level" ) )
      {
        String level = request->getParam( "level" )->value();
        elog.log( DEBUG, "%s: set-loglevel, param: %s", APIWebServer::tag, level.c_str() );
        uint8_t numLevel = static_cast< uint8_t >( level.toInt() );
        prefs::AppStati::setLogLevel( numLevel );
        request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
        yield();
        sleep( 1 );
        ESP.restart();
        return;
      }
      else
      {
        String msg = "api call v1 for <set-" + verb + "> param not found!";
        APIWebServer::onServerError( request, 303, msg );
        return;
      }
    }
    else if ( verb.equals( "interval" ) )
    {
      String interval = request->getParam( "interval" )->value();
      elog.log( DEBUG, "%s: set-interval, param: %s", APIWebServer::tag, interval.c_str() );
      uint32_t numLevel = static_cast< uint32_t >( interval.toInt() );
      if ( prefs::AppStati::setMeasureInterval_s( numLevel ) )
      {
        FileService::deleteTodayFile();
        request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
        yield();
        sleep( 1 );
        ESP.restart();
      }
      return;
    }
    else if ( verb.equals( "led" ) )
    {
      if ( request->hasParam( "brightness" ) )
      {
        String brightness = request->getParam( "brightness" )->value();
        elog.log( DEBUG, "%s: set-%s, param: %s", APIWebServer::tag, verb, brightness.c_str() );
        uint8_t br = static_cast< uint8_t >( brightness.toInt() & 0xff );
        if ( prefs::AppStati::setLedBrightness( br ) )
        {
          request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
          yield();
          sleep( 1 );
          ESP.restart();
          return;
        }
        else
        {
          request->send( 300, "text/plain", "fail api call v1 for <set-" + verb + ">" );
          return;
        }
      }
    }
    else if ( verb.equals( "fscheck" ) )
    {
      elog.log( DEBUG, "%s: set-%s, init force filesystemcheck", APIWebServer::tag, verb );
      prefs::AppStati::setForceFilesystemCheck( true );
      request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
      return;
    }
    else
    {
      request->send( 300, "text/plain", "fail api call v1 for <set-" + verb + ">" );
      return;
    }
  }
  /**
   * request for environment data for today
   */
  void APIWebServer::apiGetTodayData( AsyncWebServerRequest *request )
  {
    elog.log( DEBUG, "%s: getTodayData...", APIWebServer::tag );
    String &fileName = FileService::getTodayFileName();
    //
    // maybe their are write accesses
    //
    if ( xSemaphoreTake( FileService::measureFileSem, pdMS_TO_TICKS( 1500 ) ) == pdTRUE )
    {
      APIWebServer::deliverFileToHttpd( fileName, request );
      xSemaphoreGive( FileService::measureFileSem );
      return;
    }
    String msg = "Can't take semaphore!";
    elog.log( CRITICAL, "%s: %s", APIWebServer::tag, msg );
    APIWebServer::onServerError( request, 303, msg );
  }

  /**
   * get Datafile from date, if availible
   */
  void APIWebServer::apiGetRestDataFileFrom( AsyncWebServerRequest *request )
  {
    elog.log( DEBUG, "%s: apiGetRestDataFileFrom...", APIWebServer::tag );
    if ( request->hasParam( "from" ) )
    {
      String dateNameStr = request->getParam( "from" )->value().substring( 0, 10 );
      String fileName( "/data/" );
      fileName += dateNameStr;
      fileName += "-pressure.csv";
      elog.log( DEBUG, "%s: apiGetRestDataFileFrom try to deliver <%s>...", APIWebServer::tag, fileName.c_str() );
      if ( SPIFFS.exists( fileName ) )
      {
        //
        // maybe their are write accesses
        //
        if ( xSemaphoreTake( FileService::measureFileSem, pdMS_TO_TICKS( 1500 ) ) == pdTRUE )
        {
          APIWebServer::deliverFileToHttpd( fileName, request );
          xSemaphoreGive( FileService::measureFileSem );
          return;
        }
        String msg = "Can't take semaphore!";
        APIWebServer::onServerError( request, 303, msg );
        return;
      }
      String msg = "File <";
      msg += fileName.substring( 6 );
      msg += "> don't exist!";
      APIWebServer::onServerError( request, 303, msg );
      return;
    }
    else
    {
      String msg = "no param <from> sent!";
      APIWebServer::onServerError( request, 303, msg );
    }
  }

  /**
   * get the led stripe brightness
   */
  void APIWebServer::apiGetRestLedBrightness( AsyncWebServerRequest *request )
  {
    uint8_t br = prefs::AppStati::getLedBrightness();
    elog.log( DEBUG, "%s: apiGetRestLedBrightness (%03d)...", APIWebServer::tag, br );
    char buffer[ 18 ];
    snprintf( buffer, 18, "BRIGHTNESS: %03d", static_cast< int >( br ) );
    request->send( 200, "text/plain", buffer );
    return;
  }

  /**
   * get measure interval from server
   */
  void APIWebServer::apiGetRestInterval( AsyncWebServerRequest *request )
  {
    uint32_t interval = prefs::AppStati::getMeasureInterval_s();
    elog.log( DEBUG, "%s: apiGetRestInterval (%03d)...", APIWebServer::tag, interval );
    char buffer[ 15 ];
    snprintf( buffer, 15, "INTERVAL: %03d", static_cast< int >( interval ) );
    request->send( 200, "text/plain", buffer );
    return;
  }

  /**
   * get current flash amounts
   */
  void APIWebServer::apiGetRestFlashAmount( AsyncWebServerRequest *request )
  {
    elog.log( DEBUG, "%s: get file infos...", APIWebServer::tag );
    size_t flash_total;
    size_t flash_used;
    size_t flash_free;

    esp_err_t errorcode = esp_spiffs_info( prefs::WEB_PARTITION_LABEL, &flash_total, &flash_used );
    if ( errorcode == ESP_OK )
    {
      flash_free = flash_total - flash_used;
      elog.log( DEBUG, "%s: SPIFFS total %07d, used %07d, free %07d", APIWebServer::tag, flash_total, flash_used, flash_free );
      char buffer[ 128 ];
      snprintf( buffer, 128, "SPIFFS total %07d, used %07d, free %07d, min-free: %07d", flash_total, flash_used, flash_free,
                prefs::MIN_FILE_SYSTEM_FREE_SIZE );
      request->send( 200, "text/plain", buffer );
    }
    else
    {
      request->send( 300, "text/plain", "error while chcek spiffs space" );
    }
  }

  void APIWebServer::apiGetRestFilesystemStatus( AsyncWebServerRequest *request )
  {
    // elog.log( DEBUG, "%s: request filesystem status...", APIWebServer::tag );
    // cJSON *root = cJSON_CreateObject();
    // // total
    // String val = String( StatusObject::getFsTotalSpace(), DEC );
    // cJSON_AddStringToObject( root, "total", val.c_str() );
    // // used space
    // val = String( StatusObject::getFsUsedSpace() );
    // cJSON_AddStringToObject( root, "used", val.c_str() );
    // // dayly file
    // val = String( StatusObject::getTodayFilseSize() );
    // cJSON_AddStringToObject( root, "today", val.c_str() );
    // // weekly
    // val = String( StatusObject::getWeekFilseSize() );
    // cJSON_AddStringToObject( root, "week", val.c_str() );
    // // month
    // val = String( StatusObject::getMonthFilseSize() );
    // cJSON_AddStringToObject( root, "month", val.c_str() );
    // // weekly
    // val = String( StatusObject::getAckuFilseSize() );
    // cJSON_AddStringToObject( root, "acku", val.c_str() );
    // char *tmp_info = cJSON_PrintUnformatted( root );  // cJSON_Print(root);
    // String info( tmp_info );
    // request->send( 200, "application/json", info );
    // cJSON_Delete( root );
    // cJSON_free( tmp_info );  //!!!! important, memory leak
    // elog.log( DEBUG, "%s: request filesystem status...OK", APIWebServer::tag );
    // TODO: implement in html
    String msg = "not implemented (yet)!";
    APIWebServer::onServerError( request, 303, msg );
  }

  /**
   * deliver a file (physic pathname on the controller) via http
   */
  void APIWebServer::deliverFileToHttpd( String &filePath, AsyncWebServerRequest *request )
  {
    String contentType( "text/plain" );
    String contentTypeMarker{ 0 };

    if ( !prefs::AppStati::getIsSpiffsInit() )
    {
      elog.log( WARNING, "%s: SPIFFS not initialized, send file ABORT!", APIWebServer::tag );
      request->send( 500, "text/plain", "SPIFFS not initialized" );
      return;
    }
    //
    // next check if filename not exits
    // do this after file check, so i can overwrite this
    // behavior if an file is exist
    //
    if ( !SPIFFS.exists( filePath ) )
    {
      APIWebServer::handleNotPhysicFileSources( filePath, request );
      return;
    }
    //
    // set content type of file
    //
    contentTypeMarker = APIWebServer::setContentTypeFromFile( contentType, filePath );
    elog.log( DEBUG, "%s: file <%s>: type: <%s>...", APIWebServer::tag, filePath.c_str(), contentTypeMarker.c_str() );
    //
    // send via http server, he mak this chunked if need
    //
    AsyncWebServerResponse *response = request->beginResponse( SPIFFS, filePath, contentType, false );
    response->addHeader( "Server", "ESP Environment Server" );
    if ( contentTypeMarker.equals( "js.gz" ) )
    {
      response->addHeader( "Content-Encoding", "gzip" );
    }
    request->send( response );
  }

  /**
   * handle non-physical files
   */
  void APIWebServer::handleNotPhysicFileSources( String &filePath, AsyncWebServerRequest *request )
  {
    // TODO: implemtieren von virtuellen datenpdaden
    if ( filePath == "/version.html" )
    {
      request->send_P( 200, "text/html", version_html, APIWebServer::tProcessor );
      return;
    }
    else if ( filePath == "/info.html" )
    {
      request->send_P( 200, "text/html", info_html, APIWebServer::tProcessor );
      return;
    }
    else if ( filePath == "/intervall.html" )
    {
      request->send_P( 200, "text/html", intervall_html, APIWebServer::tProcessor );
      return;
    }

    APIWebServer::onNotFound( request );
  }

  /**
   * if there is an server error
   */
  void APIWebServer::onServerError( AsyncWebServerRequest *request, int errNo, const String &msg )
  {
    prefs::AppStati::httpActive = true;
    String myUrl( request->url() );
    elog.log( ERROR, "%s: Server ERROR: %03d - %s", APIWebServer::tag, errNo, msg.c_str() );
    request->send( errNo, "text/plain", msg );
  }

  /**
   * File-Not-Fopuind Errormessage
   */
  void APIWebServer::onNotFound( AsyncWebServerRequest *request )
  {
    prefs::AppStati::httpActive = true;
    String myUrl( request->url() );
    elog.log( WARNING, "%s: url not found <%s>", APIWebServer::tag, myUrl.c_str() );
    request->send( 404, "text/plain", "URL not found: <" + myUrl + ">" );
  }

  /**
   * find out what is the content type fron the file extension
   */
  String APIWebServer::setContentTypeFromFile( String &contentType, const String &filename )
  {
    String type = "text";

    if ( filename.endsWith( ".pdf" ) )
    {
      type = "pdf";
      contentType = "application/pdf";
      return type;
    }
    if ( filename.endsWith( ".html" ) )
    {
      type = "html";
      contentType = "text/html";
      return type;
    }
    if ( filename.endsWith( ".jpeg" ) )
    {
      type = "jpeg";
      contentType = "image/jpeg";
      return type;
    }
    if ( filename.endsWith( ".ico" ) )
    {
      type = "icon";
      contentType = "image/x-icon";
      return type;
    }
    if ( filename.endsWith( ".json" ) )
    {
      type = "json";
      contentType = "application/json";
      return type;
    }
    if ( filename.endsWith( ".jdata" ) )
    {
      // my own marker for my "raw" fileformat
      type = "jdata";
      contentType = "application/json";
      return type;
    }
    if ( filename.endsWith( "js.gz" ) )
    {
      type = "js.gz";
      contentType = "text/javascript";
      return type;
    }
    if ( filename.endsWith( ".js" ) )
    {
      type = "js";
      contentType = "text/javascript";
      return type;
    }
    if ( filename.endsWith( ".css" ) )
    {
      type = "css";
      contentType = "text/css";
      return type;
    }
    //
    // This is a limited set only
    // For any other type always set as plain text
    //
    contentType = "text/plain";
    return type;
  }

  String APIWebServer::tProcessor( const String &var )
  {
    if ( var == "APP_VERSION" )
    {
      String versionStr( prefs::VERSION );
      return versionStr;
    }
    else if ( var == "IDF_VERSION" )
    {
      String versionStr( IDF_VER );
      return versionStr;
    }
    else if ( var == "ESP_MODEL" )
    {
      esp_chip_info_t chip_info;
      esp_chip_info( &chip_info );
      String model( "-" );
      switch ( chip_info.model )
      {
        default:
        case CHIP_ESP32:
          model = "ESP32";
          break;
        case CHIP_ESP32S2:
          model = "ESP32-S2";
          break;
        case CHIP_ESP32S3:
          model = "ESP32-S3";
          break;
        case CHIP_ESP32C3:
          model = "ESP32-C3";
          break;
        case CHIP_ESP32H2:
          model = "ESP32-H2";
          break;
      }
      return model;
    }
    else if ( var == "ESP_CORES" )
    {
      esp_chip_info_t chip_info;
      esp_chip_info( &chip_info );
      String coresCount( chip_info.cores );
      return coresCount;
    }
    else if ( var == "MEASURE_INTERVAL" )
    {
      String interval( prefs::MEASURE_DIFF_TIME_S );
      return interval;
    }

    return String();
  }
}  // namespace measure_h2o
