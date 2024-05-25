#pragma once
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>

namespace measure_h2o
{

  enum WlanState : uint8_t
  {
    DISCONNECTED,
    SEARCHING,
    CONNECTED,
    TIMESYNCED,
    FAILED
  };


  struct presure_data_t
  {
    String timestamp;    //! timestamp
    uint32_t miliVolts;  //! current measured value
    float pressureBar;   //! current measured value
  };

  // name for datasets for save mesures
  using presure_data_set_t = std::vector< presure_data_t >;

}  // namespace measure_h2o

//
// <?xml version="1.0" encoding"UTF-8"?>
// <series>
//   <elem timestamp="123456789" pressure="1.45" milivolt="354" />
//   <elem timestamp="123456789" pressure="1.45" milivolt="354" />
//   <elem timestamp="123456790" pressure="1.55" milivolt="354" />
//   <elem timestamp="123456791" pressure="1.60" milivolt="354" />
//   <elem timestamp="123456792" pressure="1.35" milivolt="354" />
//   <elem timestamp="123456793" pressure="1.45" milivolt="354" />
//   <elem timestamp="123456794" pressure="1.35" milivolt="354" />
//   <elem timestamp="123456795" pressure="1.25" milivolt="354" />
//   <elem timestamp="123456796" pressure="1.45" milivolt="354" />
// </series>