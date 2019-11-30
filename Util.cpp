#include "Util.hpp"

bool Util::IsSupportedImage(const boost::filesystem::path filename) {
  auto ext = filename.extension().string();

  // TODO: case correction
  if (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG" ||
      ext == ".jpeg" || ext == ".JPEG" || ext == ".tif" || ext == ".tiff" ||
      ext == ".TIF" || ext == ".TIFF") {
    return true;
  }
  return false;
}