#include "filesystem/file_cache.h"

#include "base/client_paths.h"
#include "base/path_service.h"
#include "filesystem/file_registry.h"

#include <gtest/gtest.h>

namespace {

class FileCacheTest : public ::testing::Test {
 protected:
  FileCacheTest() {
    base::PathService::Get(client::DIR_PUBLIC, &original_public_path_);
    fallback_cache_path_ =
        std::filesystem::current_path() / "file-cache.json";
    std::filesystem::remove(fallback_cache_path_);
  }

  ~FileCacheTest() override {
    std::filesystem::remove(fallback_cache_path_);
    base::PathService::Override(client::DIR_PUBLIC, original_public_path_);
  }

  std::filesystem::path original_public_path_;
  std::filesystem::path fallback_cache_path_;
  FileRegistry file_registry_;
};

TEST_F(FileCacheTest, DoesNotWriteRelativeCacheFileWhenPublicPathMissing) {
  base::PathService::Override(client::DIR_PUBLIC, {});

  {
    FileCache file_cache{file_registry_};
    file_cache.Init();
  }

  EXPECT_FALSE(std::filesystem::exists(fallback_cache_path_));
}

}  // namespace
