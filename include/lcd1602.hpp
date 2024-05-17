#pragma once
#include <memory>
#include <Wire.h>
#include <Waveshare_LCD1602.h>

namespace measure_h2o
{
  class MLCD : public Waveshare_LCD1602
  {
    private:
    static const char *tag;

    public:
    MLCD( uint8_t lcd_cols, uint8_t lcd_rows, int sda, int scl );
  };

  using sysDisplay = std::shared_ptr< MLCD >;
}  // namespace measure_h2o