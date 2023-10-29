#pragma once

#include "aui/types.h"
#include "base/containers/span.h"
#include "base/promise.h"

#include <filesystem>
#include <string_view>

#if defined(UI_QT)
class QWidget;
#elif defined(UI_WT)
namespace Wt {
class WWidget;
}
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

  virtual UiView* GetDialogOwningWindow() const = 0;

  virtual UiView* GetParentWidget() const = 0;

  virtual promise<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                                  std::u16string_view title,
                                                  MessageBoxMode mode) = 0;

  virtual promise<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) = 0;

  struct Filter {
    std::u16string_view title;
    base::span<const std::string_view> extensions;
  };

  struct SaveParams {
    std::u16string_view title;
    std::filesystem::path default_path;
    base::span<const Filter> filters;
  };

  virtual promise<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) = 0;
};
