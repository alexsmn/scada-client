#pragma once

#include "aui/dialog_service.h"

#include <gmock/gmock.h>

class MockDialogService : public DialogService {
 public:
  MockDialogService() {
    using namespace testing;

    ON_CALL(*this, SelectOpenFile(/*title=*/_))
        .WillByDefault(Return(
            make_rejected_promise<std::filesystem::path>(std::exception{})));
  }

  MOCK_METHOD(UiView*, GetDialogOwningWindow, (), (const override));

  MOCK_METHOD(UiView*, GetParentWidget, (), (const override));

  MOCK_METHOD(promise<MessageBoxResult>,
              RunMessageBox,
              (std::u16string_view message,
               std::u16string_view title,
               MessageBoxMode mode),
              (override));

  MOCK_METHOD(promise<std::filesystem::path>,
              SelectOpenFile,
              (std::u16string_view title),
              (override));

  MOCK_METHOD(promise<std::filesystem::path>,
              SelectSaveFile,
              (const SaveParams& params),
              (override));
};
