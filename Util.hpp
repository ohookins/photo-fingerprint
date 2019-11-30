#include <boost/filesystem.hpp>

// FIXME: Find a better place for this
class Util {
public:
  static bool IsSupportedImage(const boost::filesystem::path filename);
};