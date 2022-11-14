#include "wt/dialog_service_impl_wt.h"

#include "base/strings/string_util.h"

promise<MessageBoxResult> DialogServiceImplWt::RunMessageBox(
    std::u16string_view message,
    std::u16string_view title,
    MessageBoxMode mode) {
  return make_resolved_promise(MessageBoxResult::Ok);
}

UiView* DialogServiceImplWt::GetDialogOwningWindow() const {
  return nullptr;
}

UiView* DialogServiceImplWt::GetParentWidget() const {
  return parent_widget;
}

std::filesystem::path DialogServiceImplWt::SelectOpenFile(
    std::u16string_view title) {
  return {};
}

std::filesystem::path DialogServiceImplWt::SelectSaveFile(
    const SaveParams& params) {
  return {};
}
