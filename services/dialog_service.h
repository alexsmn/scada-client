#pragma once

#include "base/strings/string_piece.h"
#include "gfx/native_widget_types.h"

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

  virtual MessageBoxResult RunMessageBox(base::StringPiece16 message,
                                         base::StringPiece16 title,
                                         MessageBoxMode mode) = 0;
};
