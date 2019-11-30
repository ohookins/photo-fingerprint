#include "Magick++.h"
#include <optional>
#include <vector>

class FingerprintStore {
public:
  void Load(const std::string srcDirectory);
  void FindMatches(Magick::Image image, const std::string filename);

  static void Generate(const std::string srcDirectory,
                       const std::string dstDirectory, const int numThreads);

private:
  static void RunGenerateWorker(DirectoryWalker *dw,
                                const boost::filesystem::path dest);

  // Store all fingerprint images in memory for now
  std::vector<std::pair<Magick::Image, std::string>> Fingerprints;

  const double DistortionThreshold = 0.01;
  const int FuzzFactor = 10000;

  // Dimension specification for comparison fingerprints.
  // ! means ignoring proportions
  inline static const std::string FingerprintSpec = "100x100!";
};