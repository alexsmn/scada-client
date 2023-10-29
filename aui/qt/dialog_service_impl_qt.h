#pragma once

#include "aui/dialog_service.h"

class DialogServiceImplQt final : public DialogService {
 public:
  // DialogService
  virtual UiView* GetDialogOwningWindow() const override;
  virtual UiView* GetParentWidget() const override;
  virtual promise<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                                  std::u16string_view title,
                                                  MessageBoxMode mode) override;
  virtual promise<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override;
  virtual promise<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override;

  QWidget* parent_widget = nullptr;
};
