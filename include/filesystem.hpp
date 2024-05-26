#pragma once
namespace measure_h2o
{
  class Filesystem
  {
    private:
    static bool wasInit;
    static bool isOkay;

    public:
    static void init();
    static bool getIsOkay()
    {
      if ( Filesystem::wasInit && Filesystem::isOkay )
        return true;
      return false;
    }
  };
}  // namespace measure_h2o
