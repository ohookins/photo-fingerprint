#include "Magick++.h"
#include <optional>
#include <vector>

class FingerprintStore {
public:
  void Load(const std::string srcDirectory);
  void FindMatches(Magick::Image image, const std::string filename);

private:
  // Store all fingerprint images in memory for now
  std::vector<std::pair<Magick::Image, std::string>> Fingerprints;

  const double DistortionThreshold = 0.01;
  const int FuzzFactor = 10000;
};