#include "Magick++.h"
#include <vector>

enum WorkerType { GenerateWorker, MetadataWorker, FingerprintWorker };

struct WorkerOptions {
  const DirectoryWalker *DW;
  const boost::filesystem::path Destination;
  const WorkerType WType;
};

class FingerprintStore {
public:
  FingerprintStore(std::string srcDirectory);

  void Load();

  // Find duplicates in a whole directory compared to the fingerprints.
  void FindDuplicates(const std::string dstDirectory, const int fuzzFactor,
                      const int numThreads);

  // Entrypoint for generating fingerprints in parallel threads
  void Generate(const std::string dstDirectory, const int numThreads);

  // Entrypoint for outputting metadata in parallel threads.
  // Currently the only metadata is the created date of the image.
  void Metadata(const int numThreads);

private:
  // Compare a single image to all of the fingerprints
  void FindMatchesForImage(Magick::Image image, const std::string filename,
                           const int fuzzFactor);

  void RunWorker(const WorkerOptions options);

  void RunDuplicateWorker(DirectoryWalker *dw, const int fuzzFactor);

  void RunGenerateWorker(DirectoryWalker *dw,
                         const boost::filesystem::path dest);

  void RunMetadataWorker(DirectoryWalker *dw);

  // Converts a timestamp like "2011:07:09 20:01:28" into a standard format
  // (hyphens between date parts).
  std::string ConvertExifTimestamp(const std::string timestamp);

  // Source directory for the given operation
  std::string SrcDirectory;

  // Store all fingerprint images in memory for now
  std::vector<std::pair<Magick::Image, std::string>> Fingerprints;

  const double LowDistortionThreshold = 10;   // identical images
  const double HighDistortionThreshold = 100; // similar images

  // Dimension specification for comparison fingerprints.
  // ! means ignoring proportions
  const std::string FingerprintSpec = "100x100!";
};