#include "wt/dialog_service_impl_wt.h"

#include "base/strings/string_util.h"

promise<MessageBoxResult> DialogServiceImplWt::RunMessageBox(
    std::wstring_view message,
    std::wstring_view title,
    MessageBoxMode mode) {
  return make_resolved_promise(MessageBoxResult::Ok);
}

gfx::NativeView DialogServiceImplWt::GetDialogOwningWindow() const {
  return nullptr;
}

Wt::WWidget* DialogServiceImplWt::GetParentWidget() const {
  return parent_widget;
}

std::filesystem::path DialogServiceImplWt::SelectOpenFile(
    std::wstring_view title) {
  return {};
}

std::filesystem::path DialogServiceImplWt::SelectSaveFile(
    const SaveParams& params) {
  return {};
}
