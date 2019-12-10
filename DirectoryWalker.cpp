#include "DirectoryWalker.hpp"
#include <iostream>
#include <queue>

DirectoryWalker::DirectoryWalker(const std::string directoryName)
    : Queue(0), Directory(directoryName) {}

void DirectoryWalker::Traverse(const bool descend = false) {
  Completed = false;

  Worker = std::thread([this, descend]() {
    std::cerr << "Retrieving list of files...\n";
    // TODO: Check that the directory is valid

    // Push the starting directory onto the queue so the loop is the same for
    // each directory
    std::queue<boost::filesystem::path> toBeListed;
    toBeListed.push(Directory);

    for (;;) {
      if (toBeListed.empty())
        break;

      boost::filesystem::path currentDir = toBeListed.front();
      toBeListed.pop();

      for (boost::filesystem::directory_entry &entry :
           boost::filesystem::directory_iterator(currentDir)) {
        // TODO: Print out number of entries walked in an ncurses window?

        // Add any found directories to the traversal queue
        if (boost::filesystem::is_directory(entry)) {
          if (descend)
            toBeListed.push(entry);

          continue;
        }

        Queue.push(new boost::filesystem::path(entry));
      }
    }
    Completed = true;
  });
}

std::pair<std::optional<boost::filesystem::path>, bool>
DirectoryWalker::GetNext() {
  boost::filesystem::path *entry;
  bool success = Queue.pop(entry);

  if (!success)
    return {std::nullopt, Completed};

  auto retval = boost::filesystem::path(*entry);
  delete entry;
  return {retval, Completed};
}

void DirectoryWalker::Finish() {
  // If the directory traversal has completed, join the thread.
  // This is probably not a great way to do this. Also because we don't actually
  // check that the traversal has completed (although if the thread has finished
  // we can assume it has).
  if (Worker.joinable()) {
    Worker.join();
  }
}