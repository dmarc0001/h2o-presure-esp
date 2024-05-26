#pragma Once
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

namespace measure_h2o
{
  class APIWebServer
  {
    private:
    static const char *tag;        //! for debugging
    static AsyncWebServer server;  //! webserver ststic

    public:
    static void init();   //! init http server
    static void start();  //! server.begin()
    static void stop();   //! server stop

    private:
    static void onIndex( AsyncWebServerRequest * );                               //! on index ("/" or "/index.html")
    static void onApiV1( AsyncWebServerRequest * );                               //! on url path "/api/v1/"
    static void onApiV1Set( AsyncWebServerRequest * );                            //! set a few things vis REST
    static void onFilesReq( AsyncWebServerRequest * );                            //! on some file
    static void apiGetTodayData( AsyncWebServerRequest * );                       //! on api get today data
    static void apiSystemInfoGetHandler( AsyncWebServerRequest * );               //! deliver server info
    static void apiVersionInfoGetHandler( AsyncWebServerRequest * );              //! deliver esp infos
    static void apiRestHandlerInterval( AsyncWebServerRequest * );                //! deliver measure interval
    static void apiRestHandlerCurrent( AsyncWebServerRequest * );                 //! deliver acku current
    static void apiRestFilesystemCheck( AsyncWebServerRequest * );                //! trigger the filesystem checker...
    static void apiRestFilesystemStatus( AsyncWebServerRequest * );               //! get an overview for filesystem as json
    static void deliverFileToHttpd( String &, AsyncWebServerRequest * );          //! deliver content file via http
    static void handleNotPhysicFileSources( String &, AsyncWebServerRequest * );  //! handle virtual files/paths
    static String setContentTypeFromFile( String &, const String & );             //! find content type
    static void onNotFound( AsyncWebServerRequest * );                            //! if page not found
    static void onServerError( AsyncWebServerRequest *, int, const String & );    //! if server error
  };

}  // namespace measure_h2o
