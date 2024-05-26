#include <memory>
#include <esp_chip_info.h>
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

  const char intervall_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html lang="de-DE">
    <head>
    <title>Messintervall</title>
    </head>
    <body>
    <h1>Intervall der Messungen</h1
    <h2>%MEASURE_INTERVAL% Sekunden</h2>
    <br />
    <br />
    <span>(c) Dirk Marciniak</span>
    </body>
  </html>
  )rawliteral";

  const char version_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html lang="de-DE">
    <head>
    <title>Server Version</title>
    </head>
    <body>
    <h1>Interne Versionsnummer</h1
    <h2>%APP_VERSION%</h2>
    <br />
    <br />
    <span>(c) Dirk Marciniak</span>
    </body>
  </html>
  )rawliteral";

  const char info_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html lang="de-DE">
    <head>
    <title>Server Information</title>
    </head>
    <body>
    <h1>Server Informationen</h1>
    <h2>IDF Version: %IDF_VERSION%</h2>
    <h2>ESP Model: %ESP_MODEL%</h2>
    <h2>CORES: %ESP_CORES%</h2>
    <br />
    <br />
    <span>(c) Dirk Marciniak</span>
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
    APIWebServer::server.on( "^\\/api\\/v1\\/(.*)$", HTTP_GET, APIWebServer::onApiV1 );
    APIWebServer::server.on( "^\\/api\\/v1\\/set-(.*)\?(.*)$", HTTP_GET, APIWebServer::onApiV1Set );
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
    prefs::AppStati::setHttpActive( true );
    APIWebServer::deliverFileToHttpd( file, request );
  }

  /**
   * response for request a file (for a directory my fail)
   */
  void APIWebServer::onFilesReq( AsyncWebServerRequest *request )
  {
    prefs::AppStati::setHttpActive( true );
    String file( request->url() );
    APIWebServer::deliverFileToHttpd( file, request );
  }

  /**
   * response for an api request, Version 1
   */
  void APIWebServer::onApiV1( AsyncWebServerRequest *request )
  {
    prefs::AppStati::setHttpActive( true );
    String parameter = request->pathArg( 0 );
    elog.log( DEBUG, "%s: api version 1 call <%s>", APIWebServer::tag, parameter );
    if ( parameter.equals( "today" ) )
    {
      APIWebServer::apiGetTodayData( request );
    }
    else if ( parameter.equals( "fsstat" ) )
    {
      APIWebServer::apiRestFilesystemStatus( request );
    }
    else
    {
      request->send( 300, "text/plain", "fail api call v1 for <" + parameter + ">" );
    }
  }

  /**
   * compute set commands via API
   */
  void APIWebServer::onApiV1Set( AsyncWebServerRequest *request )
  {
    prefs::AppStati::setHttpActive( true );
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
        prefs::AppStati::setTimeZone( timezone );
        request->send( 200, "text/plain", "OK api call v1 for <set-" + verb + ">" );
        setenv( "TZ", timezone.c_str(), 1 );
        tzset();
        yield();
        sleep( 1 );
        ESP.restart();
        return;
      }
      else
      {
        elog.log( ERROR, "%s: set-timezone, param not found!", APIWebServer::tag );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
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
        elog.log( ERROR, "%s: set-loglevel, param not found!", APIWebServer::tag );
        request->send( 300, "text/plain", "api call v1 for <set-" + verb + "> param not found!" );
        return;
      }
    }
    else if ( verb.equals( "interval" ) )
    {
      // TODO: implement!
      String msg = "not implemented (yet)!";
      APIWebServer::onServerError( request, 303, msg );
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
    // String msg = "not implemented (yet)!";
    APIWebServer::onServerError( request, 303, msg );
  }

  void APIWebServer::apiRestFilesystemStatus( AsyncWebServerRequest *request )
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
    prefs::AppStati::setHttpActive( true );
    String myUrl( request->url() );
    elog.log( ERROR, "%s: Server ERROR: %03d - %s", APIWebServer::tag, errNo, msg.c_str() );
    request->send( errNo, "text/plain", msg );
  }

  /**
   * File-Not-Fopuind Errormessage
   */
  void APIWebServer::onNotFound( AsyncWebServerRequest *request )
  {
    prefs::AppStati::setHttpActive( true );
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
