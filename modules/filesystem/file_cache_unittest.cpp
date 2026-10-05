#include "filesystem/file_cache.h"

#include "base/client_paths.h"
#include "base/path_service.h"
#include "filesystem/file_registry.h"

#include <gtest/gtest.h>

namespace {

class FileCacheTest : public ::testing::Test {
 protected:
  FileCacheTest() {
    scada::base::PathService::Get(client::DIR_PUBLIC, &original_public_path_);
    fallback_cache_path_ = std::filesystem::current_path() / "file-cache.json";
    std::filesystem::remove(fallback_cache_path_);
  }

  ~FileCacheTest() override {
    std::filesystem::remove(fallback_cache_path_);
    scada::base::PathService::Override(client::DIR_PUBLIC,
                                       original_public_path_);
  }

  std::filesystem::path original_public_path_;
  std::filesystem::path fallback_cache_path_;
  FileRegistry file_registry_;
};

TEST_F(FileCacheTest, DoesNotWriteRelativeCacheFileWhenPublicPathMissing) {
  scada::base::PathService::Override(client::DIR_PUBLIC, {});

  {
    FileCache file_cache{file_registry_};
    file_cache.Init();
  }

  EXPECT_FALSE(std::filesystem::exists(fallback_cache_path_));
}

// Which display OpenDisplayContainingNode opens is the first match, so the
// order must not depend on the order the cache happened to be filled in.
TEST(FileListTest, FilesContainingItemAreOrderedByTitleThenPath) {
  const scada::NodeId item{7, 1};
  const scada::NodeId other{8, 1};

  FileCache::FileList list;
  list.push_back(
      {.path = "c.vds", .title = u"Substation", .items = {{item, {}}}});
  list.push_back({.path = "z.vds", .title = u"Feeder", .items = {{other, {}}}});
  list.push_back({.path = "b.vds", .title = u"Feeder", .items = {{item, {}}}});
  list.push_back({.path = "a.vds", .title = u"Feeder", .items = {{item, {}}}});

  std::vector<std::filesystem::path> paths;
  for (const auto& [path, tag] : list.GetFilesContainingItem(item))
    paths.push_back(path);

  EXPECT_EQ(paths,
            (std::vector<std::filesystem::path>{"a.vds", "b.vds", "c.vds"}));
}

}  // namespace
