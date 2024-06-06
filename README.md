# watch water pressure

TODO:
  KEY for CALIBRE

  - small api for requests (return json)
  - HTTP-GET /version,html: software version
  - HTTP-GET /info.html : idf/platformio version, count of cpu cores
  - HTTP-GET /api/v1/today : measure data for round about 24 hours ago
  - HTTP-GET /api/v1/data?from=2024-06-06 : get data from 2024-06-06, if availible
  - HTTP-GET /api/v1/interval : measure interval (delete today data file)
  - HTTP-GET /api/v1/set-timezone?timezone=GMT : set timezone (not working timezone bug)
  - HTTP-GET /api/v1/set-timezone?timezone-offset=3600 : set timezone offset from GMT (workarround for timezone bug)
  - HTTP-GET /api/v1/set-loglevel?level=7 : set controller loglevel, only app
  - HTTP-GET /api/v1/set-interval?interval=10 : set measure interval
  
## liglevels (numeric)
    EMERGENCY = 0,
    ALERT = 1,
    CRITICAL = 2,
    ERROR = 3,
    WARNING = 4,
    NOTICE = 5,
    INFO = 6,
    DEBUG = 7,








  
