#pragma once

#include <gmock/gmock.h>

#include "services/dialog_service.h"

class MockDialogService : public DialogService {
 public:
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
