#pragma once

#include "services/dialog_service.h"

class DialogServiceImplQt final : public DialogService {
 public:
  // DialogService
  virtual gfx::NativeView GetDialogOwningWindow() const override;
  virtual QWidget* GetParentWidget() const override;
  virtual promise<MessageBoxResult> RunMessageBox(std::wstring_view message,
                                                  std::wstring_view title,
                                                  MessageBoxMode mode) override;
  virtual std::filesystem::path SelectOpenFile(
      std::wstring_view title) override;
  virtual std::filesystem::path SelectSaveFile(
      const SaveParams& params) override;

  QWidget* parent_widget = nullptr;
};
