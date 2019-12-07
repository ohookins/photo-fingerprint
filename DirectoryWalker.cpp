#include "DirectoryWalker.hpp"
#include <iostream>
#include <queue>

DirectoryWalker::DirectoryWalker(const std::string directoryName)
    : Queue(0), Directory(directoryName) {}

void DirectoryWalker::Traverse(const bool descend = false) {
  std::cerr << "Retrieving list of files..." << std::endl;
  int fileCount = 0;
  // TODO: Check that the directory is valid

  // Push the starting directory onto the queue so the loop is the same for each
  // directory
  std::queue<boost::filesystem::path> toBeListed;
  toBeListed.push(Directory);

  for (;;) {
    if (toBeListed.empty())
      break;

    boost::filesystem::path currentDir = toBeListed.front();
    toBeListed.pop();

    for (boost::filesystem::directory_entry &entry :
         boost::filesystem::directory_iterator(currentDir)) {
      std::cerr << "\r" << ++fileCount;
      std::flush(std::cerr);

      // Add any found directories to the traversal queue
      if (boost::filesystem::is_directory(entry)) {
        if (descend)
          toBeListed.push(entry);

        continue;
      }

      Queue.push(new boost::filesystem::path(entry));
    }
  }
  std::cerr << std::endl;
}

std::optional<boost::filesystem::path> DirectoryWalker::GetNext() {
  boost::filesystem::path *entry;
  bool success = Queue.pop(entry);

  if (!success)
    return {};

  auto retval = boost::filesystem::path(*entry);
  delete entry;
  return retval;
}
