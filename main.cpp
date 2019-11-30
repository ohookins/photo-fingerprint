#include <Magick++.h>
#include <boost/filesystem.hpp>
#include <getopt.h>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include "DirectoryWalker.hpp"
#include "FingerprintStore.hpp"
#include "Util.hpp"

// Dimension specification for comparison fingerprints.
// ! means ignoring proportions
const std::string fingerprintSpec = "100x100!";

void usage() {
  std::cerr << "photo-fingerprint:" << std::endl << std::endl;
  std::cerr << " Generate fingerprints:" << std::endl;
  std::cerr << " -g -s <source image directory> -d <destination directory for "
               "fingerprints>"
            << std::endl;
  std::cerr << std::endl;
  std::cerr << " Find duplicates:" << std::endl;
  std::cerr << " -f -s <fingerprint source dir> -d <image dir to be searched>"
            << std::endl;
  exit(1);
}

bool areDirectoriesValid(std::string srcDirectory, std::string dstDirectory) {
  // Directory sanity check
  boost::filesystem::path s(srcDirectory);
  if (!boost::filesystem::is_directory(s)) {
    std::cerr << s << " is not a directory" << std::endl;
    return false;
  }

  boost::filesystem::path d(dstDirectory);
  if (!boost::filesystem::is_directory(d)) {
    std::cerr << d << " is not a directory" << std::endl;
    return false;
  }

  return true;
}

void findDuplicates(std::string srcDirectory, std::string dstDirectory) {
  FingerprintStore fs;
  fs.Load(srcDirectory);

  int comparedCount = 0;
  std::cerr << "Comparing images:" << std::endl;

  DirectoryWalker dw(dstDirectory);
  dw.Traverse(true);
  while (true) {
    std::optional<boost::filesystem::path> entry = dw.GetNext();
    if (!entry.has_value())
      break;

    // Filter only known image suffixes
    if (!Util::IsSupportedImage(entry.value()))
      continue;

    // Read in one image, resize it to comparison specifications
    auto filename = entry.value().string();
    Magick::Image image;
    try {
      image.read(filename);
    } catch (const std::exception &e) {
      // silently skip unreadable file for the moment
      continue;
    }
    image.resize(fingerprintSpec);

    // Compare
    fs.FindMatches(image, filename);

    comparedCount++;
    std::cerr << "\r" << comparedCount;
    std::flush(std::cerr);
  }
}

void fingerprintWorker(DirectoryWalker *dw,
                       const boost::filesystem::path dest) {
  // Iterate through all files in the directory
  while (true) {
    std::optional<boost::filesystem::path> entry = dw->GetNext();
    if (!entry.has_value())
      break;

    // Filter only known image suffixes
    if (!Util::IsSupportedImage(entry.value()))
      continue;

    std::cout << entry.value().string() << std::endl;
    auto filename = entry.value().filename().replace_extension(
        ".tif"); // save fingerprints uncompressed
    Magick::Image image;

    try {
      // destination filename
      auto outputFilename = boost::filesystem::path(dest);
      outputFilename += filename;

      image.read(entry.value().string());
      image.resize(fingerprintSpec);
      image.write(outputFilename.string());
    } catch (const std::exception &e) {
      // Some already seen:
      // Magick::ErrorCorruptImage
      // Magick::ErrorMissingDelegate
      // Magick::ErrorCoder
      // Magick::WarningImage
      std::cerr << "skipping " << entry.value().string() << " " << e.what()
                << std::endl;
    }
  }
}

void generateFingerprints(std::string srcDirectory, std::string dstDirectory,
                          int numThreads) {
  boost::filesystem::path d(dstDirectory);

  DirectoryWalker *dw = new DirectoryWalker(srcDirectory);
  dw->Traverse(true);

  // Spawn threads for the actual fingerprint generation
  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread(fingerprintWorker, dw, d));
  }

  // Wait for them to finish
  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }
}

int main(int argc, char **argv) {
  // Option handling
  int ch;
  std::string srcDirectory, dstDirectory;
  bool generateMode;
  bool findDuplicateMode;
  int numThreads = std::thread::hardware_concurrency();

  while ((ch = getopt(argc, argv, "gfd:s:t:")) != -1) {
    switch (ch) {
    case 'g':
      generateMode = true;
      break;
    case 'f':
      findDuplicateMode = true;
      break;
    case 's':
      srcDirectory = optarg;
      break;
    case 'd':
      dstDirectory = optarg;
      break;
    case 't':
      numThreads = atoi(optarg);
      break;
    default:
      usage();
    }
  }

  // Incompatible modes
  if (generateMode && findDuplicateMode)
    usage();

  // Both modes require two directories
  if (srcDirectory == "" || dstDirectory == "")
    usage();

  // Check for a sensible number of threads
  if (numThreads < 1)
    usage();
  std::cerr << "Using " << numThreads << " threads of maximum "
            << std::thread::hardware_concurrency() << std::endl;

  if (!areDirectoriesValid(srcDirectory, dstDirectory))
    return 1;

  if (generateMode)
    generateFingerprints(srcDirectory, dstDirectory, numThreads);
  if (findDuplicateMode)
    findDuplicates(srcDirectory, dstDirectory);

  return 0;
}