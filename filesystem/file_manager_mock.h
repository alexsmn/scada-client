#pragma once

#include "filesystem/file_manager.h"

#include <gmock/gmock.h>

class MockFileManager : public FileManager {
 public:
  MockFileManager() {
    using namespace testing;

    ON_CALL(*this, DownloadFileFromServer(_))
        .WillByDefault(Return(make_resolved_promise()));
  }

  MOCK_METHOD(scada::status_promise<void>,
              DownloadFileFromServer,
              (const std::filesystem::path& path),
              (const override));
};
