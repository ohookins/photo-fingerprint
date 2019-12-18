#include <boost/filesystem.hpp>
#include <getopt.h>
#include <iostream>
#include <thread>

#include "DirectoryWalker.hpp"
#include "FingerprintStore.hpp"
#include "Util.hpp"

void usage() {
  std::cerr << "photo-fingerprint:" << std::endl << std::endl;
  std::cerr << " Generate fingerprints:" << std::endl;
  std::cerr << " -g -s <source image directory> -d <destination directory for "
               "fingerprints>"
            << std::endl;
  std::cerr << std::endl;
  std::cerr << " Find duplicates:" << std::endl;
  std::cerr << " -f -s <fingerprint source dir> -d <image dir to be searched> "
               "-u <fuzz factor>"
            << std::endl;
  exit(1);
}

// Directory sanity check
bool isDirectoryValid(std::string directory) {
  boost::filesystem::path d(directory);
  if (!boost::filesystem::is_directory(d)) {
    std::cerr << d << " is not a directory" << std::endl;
    return false;
  }

  return true;
}

int main(int argc, char **argv) {
  // Option handling
  int ch = 0;
  std::string srcDirectory, dstDirectory;
  bool generateMode = false;
  bool findDuplicateMode = false;
  bool metadataMode = false;
  int numThreads = std::thread::hardware_concurrency();
  int fuzzFactor = 0;

  while ((ch = getopt(argc, argv, "mgfd:s:t:u:")) != -1) {
    switch (ch) {
    case 'm':
      metadataMode = true;
      break;
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
    case 'u':
      fuzzFactor = atoi(optarg);
      break;
    default:
      usage();
    }
  }

  // Only one mode can be selected
  if (generateMode + findDuplicateMode + metadataMode != 1)
    usage();

  // Generate and find duplicate modes require two directories
  if ((generateMode || findDuplicateMode) &&
      (srcDirectory == "" || dstDirectory == ""))
    usage();

  // Check for a sensible number of threads
  if (numThreads < 1)
    usage();
  std::cerr << "Using " << numThreads << " threads of maximum "
            << std::thread::hardware_concurrency() << std::endl;

  // All modes require at least a source directory. Check it first.
  if (!isDirectoryValid(srcDirectory))
    return 1;

  if (metadataMode) {
    FingerprintStore::Metadata(srcDirectory, numThreads);
    return 0;
  }

  // Remaining modes require a destination directory
  if (!isDirectoryValid(dstDirectory))
    return 1;

  if (generateMode)
    FingerprintStore::Generate(srcDirectory, dstDirectory, numThreads);

  if (findDuplicateMode) {
    FingerprintStore fs;
    fs.Load(srcDirectory);
    fs.FindDuplicates(dstDirectory, fuzzFactor);
  }

  return 0;
}