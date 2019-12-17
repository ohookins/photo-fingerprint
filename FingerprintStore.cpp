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

  std::cout << "Loading fingerprints into memory..." << std::endl;
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
    Fingerprints.push_back(std::pair(image, entry.value().stem().string()));
    loadedCount++;
    std::stringstream msg;
    msg << "\r" << loadedCount;
    std::cout << msg.str() << std::flush;
  }

  // Wait also on the directory traversal thread to complete.
  dw.Finish();
  std::cout << "\rDONE\n" << std::flush;
}

void FingerprintStore::FindMatchesForImage(Magick::Image image,
                                           const std::string filename) {
  for (std::vector<std::pair<Magick::Image, std::string>>::iterator it =
           Fingerprints.begin();
       it != Fingerprints.end(); ++it) {

    // Compare the image to the fingerprint for total number of non-matching
    // pixels. Distortion will be the pixel count. 100x100 gives a minimum of 0
    // and max of 10000.
    image.colorFuzz(FuzzFactor);
    auto distortion = image.compare(it->first, Magick::AbsoluteErrorMetric);

    std::stringstream msg;
    msg << filename;

    if (distortion < LowDistortionThreshold) {
      msg << " is identical to " << it->second << std::endl;
      std::cout << msg.str() << std::flush;
      continue;
    }

    if (distortion < HighDistortionThreshold) {
      msg << " is similar to " << it->second << std::endl;
      std::cout << msg.str() << std::flush;
      continue;
    }
  }
}

void FingerprintStore::FindDuplicates(const std::string dstDirectory) {
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
    image.compressType(
        MagickCore::CompressionType::NoCompression); // may not be needed
    image.resize(FingerprintSpec);

    // Compare
    FindMatchesForImage(image, filename);
  }

  // Wait also on the directory traversal thread to complete.
  dw.Finish();
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

  // Wait also on the directory traversal thread to complete.
  dw->Finish();
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

    std::stringstream msg;
    msg << entry.value().string() << std::endl;
    std::cout << msg.str() << std::flush;
    auto filename = entry.value().filename().replace_extension(
        ".tif"); // save fingerprints uncompressed
    Magick::Image image;

    try {
      // destination filename
      auto outputFilename = boost::filesystem::path(dest);
      outputFilename += filename;

      image.read(entry.value().string());
      image.defineValue("quantum", "format",
                        "floating-point"); // fix HDRI comparison issues
      image.depth(32);                     // also for the HDRI stuff
      image.compressType(
          MagickCore::CompressionType::NoCompression); // may not be needed
      image.resize(FingerprintSpec);
      image.write(outputFilename.string());
    } catch (const std::exception &e) {
      // Some already seen:
      // Magick::ErrorCorruptImage
      // Magick::ErrorMissingDelegate
      // Magick::ErrorCoder
      // Magick::WarningImage
      std::stringstream msg;
      msg << "skipping " << entry.value().string() << " " << e.what()
          << std::endl;
      std::cerr << msg.str() << std::flush;
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
    if (threads[i].joinable())
      threads[i].join();
  }

  // Wait also on the directory traversal thread to complete.
  dw->Finish();
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
      std::string createdAt = image.attribute("exif:DateTimeOriginal");

      if (createdAt != "") {
        std::string timestamp = ConvertExifTimestamp(createdAt);
        std::stringstream msg;
        msg << filename << "\t" << timestamp << std::endl;
        std::cout << msg.str() << std::flush;
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
