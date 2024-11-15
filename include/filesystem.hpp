#pragma once
namespace measure_h2o
{
  class Filesystem
  {
    private:
    static bool wasInit;  //! was filesystem initzalized ?
    static bool isOkay;   //! is filesystem okay?

    public:
    static void init();      //! init this static object
    static bool getIsOkay()  //! get if the filesystem is okay
    {
      if ( Filesystem::wasInit && Filesystem::isOkay )
        return true;
      return false;
    }
  };
}  // namespace measure_h2o
