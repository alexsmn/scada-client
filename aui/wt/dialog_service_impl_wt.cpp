#include "aui/wt/dialog_service_impl_wt.h"

#include "aui/wt/dialog_stub.h"

Awaitable<MessageBoxResult> DialogServiceImplWt::RunMessageBox(
    std::u16string_view message,
    std::u16string_view title,
    MessageBoxMode mode) {
  co_return MessageBoxResult::Ok;
}

UiView* DialogServiceImplWt::GetDialogOwningWindow() const {
  return nullptr;
}

UiView* DialogServiceImplWt::GetParentWidget() const {
  return parent_widget;
}

Awaitable<std::filesystem::path> DialogServiceImplWt::SelectOpenFile(
    std::u16string_view title) {
  return aui::wt::MakeUnsupportedDialogAwaitable<std::filesystem::path>();
}

Awaitable<std::filesystem::path> DialogServiceImplWt::SelectSaveFile(
    const SaveParams& params) {
  return aui::wt::MakeUnsupportedDialogAwaitable<std::filesystem::path>();
}
