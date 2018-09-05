#pragma once

#include "services/dialog_service.h"

class DialogServiceImplQt final : public DialogService {
 public:
  // DialogService
  virtual gfx::NativeView GetDialogOwningWindow() const override;
  virtual QWidget* GetParentWidget() const override;
  virtual MessageBoxResult RunMessageBox(base::StringPiece16 message,
                                         base::StringPiece16 title,
                                         MessageBoxMode mode) override;
  virtual std::filesystem::path SelectOpenFile(
      base::StringPiece16 title) override;
  virtual std::filesystem::path SelectSaveFile(
      base::StringPiece16 title,
      const std::filesystem::path& default_path) override;

  QWidget* parent_widget = nullptr;
};
