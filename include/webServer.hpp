#pragma Once
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
// #include <PrometheusArduino.h>
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
    static void apiGetRestInterval( AsyncWebServerRequest * );                    //! on api get mesure interval
    static void apiGetRestDataFileFrom( AsyncWebServerRequest * );                //! get data file from date (if availible)
    static void apiGetRestFilesystemCheck( AsyncWebServerRequest * );             //! trigger the filesystem checker...
    static void apiGetRestFilesystemStatus( AsyncWebServerRequest * );            //! get an overview for filesystem as json
    static void apiGetRestLedBrightness( AsyncWebServerRequest * );               //! get LED Stripe brightness
    static void apiGetRestFlashAmount( AsyncWebServerRequest * );                 //! get flash amount's
    static void onGetMetrics( AsyncWebServerRequest * );                          //! get sensors metrics
    static void deliverFileToHttpd( String &, AsyncWebServerRequest * );          //! deliver content file via http
    static void handleNotPhysicFileSources( String &, AsyncWebServerRequest * );  //! handle virtual files/paths
    static String setContentTypeFromFile( String &, const String & );             //! find content type
    static void onNotFound( AsyncWebServerRequest * );                            //! if page not found
    static void onServerError( AsyncWebServerRequest *, int, const String & );    //! if server error
    static String tProcessor( const String & );                                   //! minimalistic template processor
  };

}  // namespace measure_h2o
