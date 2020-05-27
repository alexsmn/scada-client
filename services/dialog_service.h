#pragma once

#include "base/containers/span.h"
#include "base/strings/string_piece.h"
#include "gfx/native_widget_types.h"

#include <filesystem>

#if defined(UI_QT)
class QWidget;
#endif

enum class MessageBoxMode {
  Info,
  Error,
  QuestionYesNo,
  QuestionYesNoDefaultNo,
  Count
};

enum class MessageBoxResult { Ok, Cancel, Yes, No };

class DialogService {
 public:
  virtual ~DialogService() {}

  virtual gfx::NativeView GetDialogOwningWindow() const = 0;

#if defined(UI_QT)
  virtual QWidget* GetParentWidget() const = 0;
#endif

  virtual MessageBoxResult RunMessageBox(base::StringPiece16 message,
                                         base::StringPiece16 title,
                                         MessageBoxMode mode) = 0;

  virtual std::filesystem::path SelectOpenFile(base::StringPiece16 title) = 0;

  struct Filter {
    base::StringPiece16 title;
    base::span<const base::StringPiece> extensions;
  };

  struct SaveParams {
    base::StringPiece16 title;
    std::filesystem::path default_path;
    base::span<const Filter> filters;
  };

  virtual std::filesystem::path SelectSaveFile(const SaveParams& params) = 0;
};
