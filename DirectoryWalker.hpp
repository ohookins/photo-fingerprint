#include <boost/filesystem.hpp>
#include <boost/lockfree/queue.hpp>
#include <optional>
#include <thread>

class DirectoryWalker {
public:
  DirectoryWalker(const std::string directoryName);

  // Starts the asynchronous directory traversal in a separate thread.
  // Directory entries can immediately be retrieved using GetNext();
  void Traverse(const bool descend);

  // GetNext returns a pair of values -
  // an optional next path that has been retrieved from filesystem
  // traversal, and a bool indicating if the overall traversal process
  // has completed or not.
  std::pair<std::optional<boost::filesystem::path>, bool> GetNext();

private:
  boost::filesystem::path Directory;
  boost::lockfree::queue<boost::filesystem::path *,
                         boost::lockfree::fixed_sized<false>>
      Queue;

  // Semaphore indicating that directory traversal has completed
  bool Completed = false;

  // Reference to thread running the directory traversal
  std::thread Worker;
};