#pragma once

#include "aui/types.h"
#include "base/awaitable.h"

#include <filesystem>
#include <span>
#include <string_view>

#if defined(UI_QT)
class QWidget;
#endif

enum class MessageBoxMode {
  Info,
  Error,
  QuestionYesNo,
  QuestionYesNoDefaultNo,
  // Pending changes are about to be lost: Apply (the default), Discard, or
  // Cancel. Answers `Apply`, `Discard` or `Cancel`; Escape is `Cancel`.
  QuestionApplyDiscardCancel,
  // A change described in the message is about to be made: Apply or Cancel,
  // Cancel the default and Escape. Answers `Apply` or `Cancel`. For a
  // question whose «Да/Нет» would make the operator re-read the text to learn
  // what «Да» does (docs/client/ux/dialogs.md §3).
  QuestionApplyCancel,
  Count
};

enum class MessageBoxResult { Ok, Cancel, Yes, No, Apply, Discard };

class DialogService {
 public:
  virtual ~DialogService() {}

  virtual UiView* GetDialogOwningWindow() const = 0;

  virtual UiView* GetParentWidget() const = 0;

  virtual Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                                    std::u16string_view title,
                                                    MessageBoxMode mode) = 0;

  virtual Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) = 0;

  struct Filter {
    std::u16string_view title;
    std::span<const std::string_view> extensions;
  };

  struct SaveParams {
    std::u16string_view title;
    std::filesystem::path default_path;
    std::span<const Filter> filters;
  };

  virtual Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) = 0;
};
