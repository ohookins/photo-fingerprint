#include "Magick++.h"
#include <vector>

class FingerprintStore {
public:
  void Load(const std::string srcDirectory);

  // Find duplicates in a whole directory compared to the fingerprints.
  void FindDuplicates(const std::string dstDirectory);

  // Runner for generating fingerprints in parallel threads
  static void Generate(const std::string srcDirectory,
                       const std::string dstDirectory, const int numThreads);

  // Runner for outputting metadata in parallel threads.
  // Currently the only metadata is the created date of the image.
  static void Metadata(const std::string srcDirectory, const int numThreads);

private:
  // Compare a single image to all of the fingerprints
  void FindMatchesForImage(Magick::Image image, const std::string filename);

  static void RunGenerateWorker(DirectoryWalker *dw,
                                const boost::filesystem::path dest);

  static void RunMetadataWorker(DirectoryWalker *dw);

  // Converts a timestamp like "2011:07:09 20:01:28" into a standard format
  // (hyphens between date parts).
  static std::string ConvertExifTimestamp(const std::string timestamp);

  // Store all fingerprint images in memory for now
  std::vector<std::pair<Magick::Image, std::string>> Fingerprints;

  const double DistortionThreshold = 0.01;
  const int FuzzFactor = 10000;

  // Dimension specification for comparison fingerprints.
  // ! means ignoring proportions
  inline static const std::string FingerprintSpec = "100x100!";
};