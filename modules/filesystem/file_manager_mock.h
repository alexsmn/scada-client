#pragma once

#include "base/awaitable.h"
#include "base/any_executor.h"
#include "filesystem/file_manager.h"

#include <gmock/gmock.h>

class MockFileManager : public FileManager {
 public:
  MockFileManager() {
    using namespace testing;

    ON_CALL(*this, DownloadFileFromServer(_))
        .WillByDefault(
            [](const std::filesystem::path&) -> Awaitable<void> { co_return; });
  }

 public:
  MOCK_METHOD(Awaitable<void>,
              DownloadFileFromServer,
              (const std::filesystem::path& path),
              (const override));
};
