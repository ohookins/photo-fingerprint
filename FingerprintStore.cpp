#include "DirectoryWalker.hpp"
#include "Util.hpp"
#include <boost/filesystem.hpp>
#include <iostream>

#include "FingerprintStore.hpp"

void FingerprintStore::Load(const std::string srcDirectory) {
  // Iterate through all files in the directory
  DirectoryWalker dw(srcDirectory);
  dw.Traverse(true);

  std::cout << "Loading fingerprints into memory... " << std::endl;
  int loadedCount = 0;

  while (true) {
    std::optional<boost::filesystem::path> entry = dw.GetNext();
    if (!entry.has_value())
      break;

    // Filter only known image suffixes
    if (!Util::IsSupportedImage(entry.value()))
      continue;

    auto filename = entry.value().string();
    Magick::Image image;

    image.read(filename);
    // Fingerprints.push_back(std::pair(image, filename));
    Fingerprints.push_back(std::pair(image, entry.value().stem().string()));
    loadedCount++;
    std::cout << "\r" << loadedCount;
    std::flush(std::cout);
  }

  std::cout << "\rDONE" << std::endl;
}

// Compare the image to all of the fingerprints
void FingerprintStore::FindMatches(Magick::Image image,
                                   const std::string filename) {
  for (std::vector<std::pair<Magick::Image, std::string>>::iterator it =
           Fingerprints.begin();
       it != Fingerprints.end(); ++it) {

    image.colorFuzz(FuzzFactor);
    double distortion = image.compare(it->first, Magick::AbsoluteErrorMetric);
    if (distortion < DistortionThreshold) {
      std::cout << filename << " matches fingerprint of " << it->second
                << std::endl;
    }
  }
}