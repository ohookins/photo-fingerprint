#include <Magick++.h>
#include <boost/filesystem.hpp>
#include <getopt.h>
#include <iostream>
#include <vector>

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
  boost::filesystem::path s(srcDirectory);
  int loadedCount = 0;

  std::cout << "Loading fingerprints into memory... " << std::endl;

  // Iterate through all files in the directory
  for (boost::filesystem::directory_entry &inputFilename :
       boost::filesystem::directory_iterator(s)) {
    // All fingerprints should be in a single directory, so don't descend
    // further.
    if (boost::filesystem::is_directory(inputFilename))
      continue;

    // Filter only known image suffixes
    if (!isSupportedImage(inputFilename.path()))
      continue;

    auto filename = inputFilename.path().filename();
    Magick::Image image;

    image.read(inputFilename.path().string());
    fingerprints.push_back(image);
    loadedCount++;
    std::cout << "\r" << loadedCount;
    std::flush(std::cout);
  }

  std::cout << "\rDONE" << std::endl;
}

void findDuplicates(std::string srcDirectory, std::string dstDirectory) {
  boost::filesystem::path d(dstDirectory);

  loadFingerprints(srcDirectory);
  int comparedCount;
  std::cerr << "Comparing images:" << std::endl;

  for (boost::filesystem::directory_entry &inputFilename :
       boost::filesystem::directory_iterator(d)) {
    // Only consider images to compare from a single directory for now.
    if (boost::filesystem::is_directory(inputFilename))
      continue;

    // Filter only known image suffixes
    if (!isSupportedImage(inputFilename.path()))
      continue;

    // Read in one image, resize it to comparison specifications
    auto filename = inputFilename.path().filename();
    Magick::Image image;
    try {
      image.read(inputFilename.path().string());
    } catch (const std::exception &e) {
      // silently skip unreadable file for the moment
      continue;
    }
    image.resize(fingerprintSpec);

    // Compare the image to all of the fingerprints
    for (std::vector<Magick::Image>::iterator it = fingerprints.begin();
         it != fingerprints.end(); ++it) {
      bool imagesAreIdentical = image.compare(*it);
      if (imagesAreIdentical) {
        // FIXME: Need to store the fingerprint source filename somewhere and
        // print it.
        std::cout << filename.string() << " matches fingerprint of ???"
                  << std::endl;
      }
    }

    comparedCount++;
    std::cerr << "\r" << comparedCount;
    std::flush(std::cerr);
  }
}

void generateFingerprints(std::string srcDirectory, std::string dstDirectory) {
  boost::filesystem::path s(srcDirectory);
  boost::filesystem::path d(dstDirectory);

  // Iterate through all files in the directory
  for (boost::filesystem::directory_entry &inputFilename :
       boost::filesystem::directory_iterator(s)) {
    // Don't descend into subdirectories for the moment
    if (boost::filesystem::is_directory(inputFilename))
      continue;

    // Filter only known image suffixes
    if (!isSupportedImage(inputFilename.path()))
      continue;

    std::cout << inputFilename.path() << std::endl;
    auto filename = inputFilename.path().filename();
    Magick::Image image;

    try {
      // destination filename
      auto outputFilename = d;
      outputFilename += filename;

      image.read(inputFilename.path().string());
      image.resize(fingerprintSpec);
      image.write(outputFilename.string());
    } catch (const std::exception &e) {
      // Some already seen:
      // Magick::ErrorCorruptImage
      // Magick::ErrorMissingDelegate
      // Magick::ErrorCoder
      // Magick::WarningImage
      std::cerr << "skipping " << inputFilename.path() << " " << e.what()
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