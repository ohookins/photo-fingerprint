#include <boost/filesystem.hpp>
#include <boost/lockfree/queue.hpp>
#include <optional>

class DirectoryWalker {
public:
  DirectoryWalker(const std::string directoryName);

  void Traverse(const bool descend);
  std::optional<boost::filesystem::path> GetNext();

private:
  boost::filesystem::path Directory;
  boost::lockfree::queue<boost::filesystem::path *,
                         boost::lockfree::fixed_sized<false>>
      Queue;
};