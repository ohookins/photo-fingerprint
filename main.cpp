#include <Magick++.h>
#include <boost/filesystem.hpp>
#include <getopt.h>
#include <iostream>
#include <vector>

#include "DirectoryWalker.hpp"

// Dimension specification for comparison fingerprints.
// ! means ignoring proportions
const std::string fingerprintSpec = "100x100!";

// Store all fingerprint images in memory for now
std::vector<Magick::Image> fingerprints;

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

bool isSupportedImage(boost::filesystem::path filename) {
  auto ext = filename.extension().string();

  // TODO: case correction
  if (ext == ".jpg" || ext == ".JPG" || ext == ".png" || ext == ".PNG" ||
      ext == ".jpeg" || ext == ".JPEG") {
    return true;
  }
  return false;
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

void loadFingerprints(std::string srcDirectory) {
  int loadedCount = 0;

  std::cout << "Loading fingerprints into memory... " << std::endl;

  // Iterate through all files in the directory
  DirectoryWalker dw(srcDirectory);
  dw.Traverse(true);
  while (true) {
    boost::filesystem::path* entry = dw.GetNext();
    if (entry == nullptr) break;

    // Filter only known image suffixes
    if (!isSupportedImage(*entry))
      continue;

    auto filename = entry->filename();
    Magick::Image image;

    image.read(entry->string());
    fingerprints.push_back(image);
    loadedCount++;
    std::cout << "\r" << loadedCount;
    std::flush(std::cout);
  }

  std::cout << "\rDONE" << std::endl;
}

void findDuplicates(std::string srcDirectory, std::string dstDirectory) {
  loadFingerprints(srcDirectory);
  int comparedCount;
  std::cerr << "Comparing images:" << std::endl;

  DirectoryWalker dw(dstDirectory);
  dw.Traverse(true);
  while (true) {
    boost::filesystem::path* entry = dw.GetNext();
    if (entry == nullptr) break;

    // Filter only known image suffixes
    if (!isSupportedImage(*entry))
      continue;

    // Read in one image, resize it to comparison specifications
    auto filename = entry->filename();
    Magick::Image image;
    try {
      image.read(entry->string());
    } catch (const std::exception &e) {
      // silently skip unreadable file for the moment
      continue;
    }
    image.resize(fingerprintSpec);

    // Compare the image to all of the fingerprints
    for (std::vector<Magick::Image>::iterator it = fingerprints.begin();
         it != fingerprints.end(); ++it) {
      image.colorFuzz(0.001);
      double errorCount = image.compare(*it, Magick::MeanAbsoluteErrorMetric);
      if (errorCount < 0.01) {
        // FIXME: Need to store the fingerprint source filename somewhere and
        // print it.
        std::cout << filename.string() << " matches fingerprint of ??? "
                  << errorCount << std::endl;
      }
    }

    comparedCount++;
    std::cerr << "\r" << comparedCount;
    std::flush(std::cerr);
  }
}

void generateFingerprints(std::string srcDirectory, std::string dstDirectory) {
  boost::filesystem::path d(dstDirectory);

  DirectoryWalker dw(srcDirectory);
  dw.Traverse(true);

  // Iterate through all files in the directory
  while (true) {
    boost::filesystem::path* entry = dw.GetNext();
    if (entry == nullptr) break;

    // Filter only known image suffixes
    if (!isSupportedImage(*entry))
      continue;

    std::cout << entry->string() << std::endl;
    auto filename = entry->filename();
    Magick::Image image;

    try {
      // destination filename
      auto outputFilename = d;
      outputFilename += filename;

      image.read(entry->string());
      image.resize(fingerprintSpec);
      image.write(outputFilename.string());
    } catch (const std::exception &e) {
      // Some already seen:
      // Magick::ErrorCorruptImage
      // Magick::ErrorMissingDelegate
      // Magick::ErrorCoder
      // Magick::WarningImage
      std::cerr << "skipping " << entry->string() << " " << e.what()
                << std::endl;
    }
  }
}

int main(int argc, char **argv) {
  // Option handling
  int ch;
  std::string srcDirectory, dstDirectory;
  bool generateMode;
  bool findDuplicateMode;

  while ((ch = getopt(argc, argv, "gfd:s:")) != -1) {
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
    default:
      usage();
    }
  }

  // Incompatible modes
  if (generateMode && findDuplicateMode) {
    usage();
  }

  // Both modes require two directories
  if (srcDirectory == "" || dstDirectory == "") {
    usage();
  }

  if (!areDirectoriesValid(srcDirectory, dstDirectory))
    return 1;

  if (generateMode)
    generateFingerprints(srcDirectory, dstDirectory);
  if (findDuplicateMode)
    findDuplicates(srcDirectory, dstDirectory);

  return 0;
}