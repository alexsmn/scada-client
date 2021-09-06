#pragma once

#include "services/dialog_service.h"

class DialogServiceImplWt final : public DialogService {
 public:
  // DialogService
  virtual gfx::NativeView GetDialogOwningWindow() const override;
  virtual Wt::WWidget* GetParentWidget() const override;
  virtual void RunMessageBox(std::wstring_view message,
                             std::wstring_view title,
                             MessageBoxMode mode,
                             const MessageBoxCallback& callback) override;
  virtual std::filesystem::path SelectOpenFile(
      std::wstring_view title) override;
  virtual std::filesystem::path SelectSaveFile(
      const SaveParams& params) override;

  Wt::WWidget* parent_widget = nullptr;
};
