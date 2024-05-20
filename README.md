# watch water pressure

TODO:
- small api for requests (return json)
  - HTTP-GET /api/v1/version: software version
  - HTTP-GET /api/v1/info : idf/platformio version, count of cpu cores
  - HTTP-GET /api/v1/set-timezone?timezone=GMT : set timezone (not working timezone bug)
  - HTTP-GET /api/v1/set-timezone?timezone-offset=3600 : set timezone offset from GMT (workarround for timezone bug)
  - HTTP-GET /api/v1/set-loglevel?level=7 : set controller loglevel, only app
  - HTTP-GET /api/v1/set-syslog?server=192.168.1.44&port=512 : set syslog destination (UDP), IP, Port  / IP 0 disable
  


## liglevels (numeric)
    EMERGENCY = 0,
    ALERT = 1,
    CRITICAL = 2,
    ERROR = 3,
    WARNING = 4,
    NOTICE = 5,
    INFO = 6,
    DEBUG = 7,








  
