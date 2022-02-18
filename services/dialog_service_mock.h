#pragma once

#include <gmock/gmock.h>

#include "services/dialog_service.h"

class MockDialogService : public DialogService {
 public:
  MOCK_METHOD(gfx::NativeView, GetDialogOwningWindow, (), (const override));

#if defined(UI_QT)
  MOCK_METHOD(QWidget*, GetParentWidget, (), (const override));
#elif defined(UI_WT)
  MOCK_METHOD(Wt::WWidget*, GetParentWidget, (), (const override));
#endif

  MOCK_METHOD(promise<MessageBoxResult>,
              RunMessageBox,
              (std::u16string_view message,
               std::u16string_view title,
               MessageBoxMode mode),
              (override));

  MOCK_METHOD(std::filesystem::path,
              SelectOpenFile,
              (std::u16string_view title),
              (override));

  MOCK_METHOD(std::filesystem::path,
              SelectSaveFile,
              (const SaveParams& params),
              (override));
};
