#include "DirectoryWalker.hpp"
#include <iostream>

DirectoryWalker::DirectoryWalker(std::string directoryName) {
  Directory = boost::filesystem::path(directoryName);
}

void DirectoryWalker::Traverse(bool descend = false) {
  std::cerr << "Retrieving list of files..." << std::endl;
  int fileCount;
  // TODO: Check that the directory is valid

  // Push the starting directory onto the queue so the loop is the same for each
  // directory
  std::queue<boost::filesystem::path> toBeListed;
  toBeListed.push(Directory);

  // Lock while we are putting files on the processing queue
  const std::lock_guard<std::mutex> lg(Lock);

  for (;;) {
    if (toBeListed.empty())
      break;

    boost::filesystem::path currentDir = toBeListed.front();
    toBeListed.pop();

    for (boost::filesystem::directory_entry &entry :
         boost::filesystem::directory_iterator(currentDir)) {
      std::cerr << "\r" << ++fileCount;
      // Add any found directories to the traversal queue
      if (boost::filesystem::is_directory(entry)) {
        if (descend)
          toBeListed.push(entry);

        continue;
      }

      Queue.push(entry);
    }
  }
  std::cerr << std::endl;
}

std::optional<boost::filesystem::path> DirectoryWalker::GetNext() {
  const std::lock_guard<std::mutex> lg(Lock); // RAII

  if (Queue.empty())
    return {};

  auto retval = Queue.front();
  Queue.pop();
  return boost::filesystem::path(retval);
}
