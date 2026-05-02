#pragma once

#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "filesystem/file_manager.h"

#include <gmock/gmock.h>

class MockFileManager : public FileManager {
 public:
  MockFileManager() {
    using namespace testing;

    ON_CALL(*this, DownloadFileFromServer(_))
        .WillByDefault([](const std::filesystem::path&) {
          return ToPromise(MakeThreadAnyExecutor(), ResolveDownloadAsync());
        });
  }

 private:
  static Awaitable<void> ResolveDownloadAsync() {
    co_return;
  }

 public:
  MOCK_METHOD(promise<void>,
              DownloadFileFromServer,
              (const std::filesystem::path& path),
              (const override));
};
