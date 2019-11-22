#include <iostream>
#include <boost/filesystem.hpp>
#include <Magick++.h>
#include <getopt.h>

void usage() {
  std::cerr << "photo-fingerprint:" << std::endl;
  std::cerr << " -s <directory>" << std::endl;
  std::cerr << " -d <directory>" << std::endl;
  exit(1);
}

void traverseDirectory(std::string srcDirectory, std::string dstDirectory) {
  // Directory sanity check
  boost::filesystem::path s(srcDirectory);
  if (!boost::filesystem::is_directory(s)) {
    std::cerr << s << " is not a directory" << std::endl;
    return;
  }

  boost::filesystem::path d(dstDirectory);
  if (!boost::filesystem::is_directory(d)) {
    std::cerr << d << " is not a directory" << std::endl;
    return;
  }

  // Iterate through all files in the directory
  for (boost::filesystem::directory_entry& inputFilename : boost::filesystem::directory_iterator(s)) {
    // Don't descend into subdirectories for the moment
    if (boost::filesystem::is_directory(inputFilename)) continue;

    std::cout << inputFilename.path() << std::endl;
    auto filename = inputFilename.path().filename();
    Magick::Image image;

    try
    {
      // destination filename
      auto outputFilename = d;
      outputFilename += filename;

      image.read(inputFilename.path().string());
      image.resize("100x100!"); // ! means ignoring proportions
      image.write(outputFilename.string());
    }
    catch(const Magick::ErrorMissingDelegate& e)
    {
      std::cerr << "skipping " << inputFilename.path() << " " << e.what() << std::endl;
    }
    catch(const Magick::ErrorCoder& e)
    {
      std::cerr << "skipping " << inputFilename.path() << " " << e.what() << std::endl;
    }
  }
}

int main(int argc, char** argv) {
  // Option handling
  int ch;
  std::string srcDirectory, dstDirectory;
  while ((ch = getopt(argc, argv, "d:s:")) != -1) {
    switch(ch) {
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

  if (srcDirectory == "" || dstDirectory == "") {
    usage();
  }

  traverseDirectory(srcDirectory, dstDirectory);

  return 0;
}