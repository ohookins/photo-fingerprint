#include "Magick++.h"
#include <vector>

enum WorkerType { GenerateWorker, MetadataWorker, FingerprintWorker };

struct WorkerOptions {
  int NumThreads;
  int FuzzFactor;
  std::string DstDirectory;
  WorkerType WType;
};

class FingerprintStore {
public:
  FingerprintStore(std::string srcDirectory);

  void Load();

  // Run a given task in multiple threads.
  void RunWorkers(const WorkerOptions options);

private:
  // Compare a single image to all of the fingerprints
  void FindMatchesForImage(Magick::Image image, const std::string filename,
                           const int fuzzFactor);

  // Find duplicates in a whole directory compared to the fingerprints.
  void FindDuplicates(DirectoryWalker *dw, const int fuzzFactor);

  // Entrypoint for generating fingerprints in parallel threads
  void Generate(DirectoryWalker *dw, const std::string dstDirectory);

  // Worker for outputting metadata.
  // Currently the only metadata is the created date of the image.
  void ExtractMetadata(DirectoryWalker *dw);

  // Converts a timestamp like "2011:07:09 20:01:28" into a standard format
  // (hyphens between date parts).
  std::string ConvertExifTimestamp(const std::string timestamp);

  // Source directory for the given operation
  std::string SrcDirectory;

  // Store all fingerprint images in memory for now
  std::vector<std::pair<Magick::Image, std::string>> Fingerprints;

  const double LowDistortionThreshold = 0.01;  // identical images
  const double HighDistortionThreshold = 0.02; // similar images

  // Dimension specification for comparison fingerprints.
  // ! means ignoring proportions
  const std::string FingerprintSpec = "100x100!";
};