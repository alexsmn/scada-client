#pragma once

#include "base/containers/span.h"
#include "base/promise.h"
#include "gfx/native_widget_types.h"

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

  virtual gfx::NativeView GetDialogOwningWindow() const = 0;

#if defined(UI_QT)
  virtual QWidget* GetParentWidget() const = 0;
#elif defined(UI_WT)
  virtual Wt::WWidget* GetParentWidget() const = 0;
#endif

  virtual promise<MessageBoxResult> RunMessageBox(std::wstring_view message,
                                                  std::wstring_view title,
                                                  MessageBoxMode mode) = 0;

  virtual std::filesystem::path SelectOpenFile(std::wstring_view title) = 0;

  struct Filter {
    std::wstring_view title;
    base::span<const std::string_view> extensions;
  };

  struct SaveParams {
    std::wstring_view title;
    std::filesystem::path default_path;
    base::span<const Filter> filters;
  };

  virtual std::filesystem::path SelectSaveFile(const SaveParams& params) = 0;
};
