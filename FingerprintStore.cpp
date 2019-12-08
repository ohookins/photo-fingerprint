#include "DirectoryWalker.hpp"
#include "Util.hpp"
#include <boost/filesystem.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include "FingerprintStore.hpp"

void FingerprintStore::Load(const std::string srcDirectory) {
  // Start iteration through all files in the directory
  DirectoryWalker dw(srcDirectory);
  dw.Traverse(true);

  std::cout << "Loading fingerprints into memory... " << std::endl;
  int loadedCount = 0;

  while (true) {
    auto next = dw.GetNext();
    std::optional<boost::filesystem::path> entry = next.first;
    bool completed = next.second;

    // No next value as the directory traversal has completed.
    if (!entry.has_value() && completed)
      break;

    // We may not have an entry, but if directory traversal hasn't completed, we
    // should wait for the next one.
    if (!entry.has_value() && !completed) {
      sleep(1);
      continue;
    }

    // Filter only known image suffixes
    if (!Util::IsSupportedImage(entry.value()))
      continue;

    auto filename = entry.value().string();
    Magick::Image image;

    image.read(filename);
    // Fingerprints.push_back(std::pair(image, filename));
    Fingerprints.push_back(std::pair(image, entry.value().stem().string()));
    loadedCount++;
    std::cout << "\r" << loadedCount;
    std::cout.flush();
  }

  std::cout << "\rDONE" << std::endl;
}

void FingerprintStore::FindMatchesForImage(Magick::Image image,
                                           const std::string filename) {
  for (std::vector<std::pair<Magick::Image, std::string>>::iterator it =
           Fingerprints.begin();
       it != Fingerprints.end(); ++it) {

    image.colorFuzz(FuzzFactor);
    double distortion = image.compare(it->first, Magick::AbsoluteErrorMetric);
    if (distortion < DistortionThreshold) {
      std::cout << filename << " matches fingerprint of " << it->second
                << std::endl;
    }
  }
}

void FingerprintStore::FindDuplicates(const std::string dstDirectory) {
  int comparedCount = 0;
  std::cerr << "Comparing images:" << std::endl;

  // Start asynchronous traversal of directory.
  DirectoryWalker dw(dstDirectory);
  dw.Traverse(true);

  while (true) {
    auto next = dw.GetNext();
    std::optional<boost::filesystem::path> entry = next.first;
    bool completed = next.second;

    // No next value as the directory traversal has completed.
    if (!entry.has_value() && completed)
      break;

    // We may not have an entry, but if directory traversal hasn't completed, we
    // should wait for the next one.
    if (!entry.has_value() && !completed) {
      sleep(1);
      continue;
    }

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
    image.resize(FingerprintSpec);

    // Compare
    FindMatchesForImage(image, filename);

    comparedCount++;
    std::cerr << "\r" << comparedCount;
    std::cerr.flush();
  }
}

void FingerprintStore::Generate(const std::string srcDirectory,
                                const std::string dstDirectory,
                                const int numThreads) {
  boost::filesystem::path d(dstDirectory);

  // Start asynchronous traversal of source directory.
  DirectoryWalker *dw = new DirectoryWalker(srcDirectory);
  dw->Traverse(true);

  // Spawn threads for the actual fingerprint generation
  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread(RunGenerateWorker, dw, d));
  }

  // Wait for them to finish
  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }
}

void FingerprintStore::RunGenerateWorker(DirectoryWalker *dw,
                                         const boost::filesystem::path dest) {
  // Iterate through all files in the directory
  while (true) {
    auto next = dw->GetNext();
    std::optional<boost::filesystem::path> entry = next.first;
    bool completed = next.second;

    // No next value as the directory traversal has completed.
    if (!entry.has_value() && completed)
      break;

    // We may not have an entry, but if directory traversal hasn't completed, we
    // should wait for the next one.
    if (!entry.has_value() && !completed) {
      sleep(1);
      continue;
    }

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
      image.resize(FingerprintSpec);
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

void FingerprintStore::Metadata(const std::string srcDirectory,
                                const int numThreads) {
  // Start asynchronous traversal of source directory
  DirectoryWalker *dw = new DirectoryWalker(srcDirectory);
  dw->Traverse(true);

  // Spawn threads for the actual fingerprint generation
  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; i++) {
    threads.push_back(std::thread(RunMetadataWorker, dw));
  }

  // Wait for them to finish
  for (int i = 0; i < numThreads; i++) {
    threads[i].join();
  }
}

void FingerprintStore::RunMetadataWorker(DirectoryWalker *dw) {
  // Iterate through all files in the directory
  while (true) {
    auto next = dw->GetNext();
    std::optional<boost::filesystem::path> entry = next.first;
    bool completed = next.second;

    // No next value as the directory traversal has completed.
    if (!entry.has_value() && completed)
      break;

    // We may not have an entry, but if directory traversal hasn't completed, we
    // should wait for the next one.
    if (!entry.has_value() && !completed) {
      sleep(1);
      continue;
    }

    // Filter only known image suffixes
    if (!Util::IsSupportedImage(entry.value()))
      continue;

    try {
      Magick::Image image;
      std::string filename = entry.value().string();
      image.read(filename);
      std::string createdAt = image.attribute("exif:DateTime");

      if (createdAt != "") {
        std::string timestamp = ConvertExifTimestamp(createdAt);
        std::cout << filename << "\t" << timestamp << std::endl;
        std::cout.flush();
      }
    } catch (const std::exception &e) {
      // Some already seen:
      // Magick::ErrorCorruptImage
      // Magick::ErrorMissingDelegate
      // Magick::ErrorCoder
      // Magick::WarningImage
      // Don't bother printing anything as we might run into all kinds of files
      // we can't read.
    }
  }
}

std::string
FingerprintStore::ConvertExifTimestamp(const std::string timestamp) {
  // https://en.cppreference.com/w/cpp/io/manip/get_time
  // This is basically the example from the docs.
  std::tm t;
  std::istringstream ss(timestamp);
  ss >> std::get_time(&t, "%Y:%m:%d %H:%M:%S");

  // std::put_time requires an output stream to write to
  std::ostringstream output;
  output << std::put_time(&t, "%Y-%m-%d %H:%M:%S");
  return output.str();
}
