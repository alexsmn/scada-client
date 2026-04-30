#pragma once

#include "aui/dialog_service.h"

class DialogServiceImplWt final : public DialogService {
 public:
  // DialogService
  virtual UiView* GetDialogOwningWindow() const override;
  virtual UiView* GetParentWidget() const override;
  virtual Awaitable<MessageBoxResult> RunMessageBox(
      std::u16string_view message,
      std::u16string_view title,
      MessageBoxMode mode) override;
  virtual Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override;
  virtual Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override;

  Wt::WWidget* parent_widget = nullptr;
};
