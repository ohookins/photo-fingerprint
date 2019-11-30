#include <boost/filesystem.hpp>
#include <mutex>
#include <optional>
#include <queue>

class DirectoryWalker {
public:
  DirectoryWalker(const std::string directoryName);

  void Traverse(const bool descend);
  std::optional<boost::filesystem::path> GetNext();

private:
  boost::filesystem::path Directory;
  std::mutex Lock;
  std::queue<boost::filesystem::path> Queue;
};