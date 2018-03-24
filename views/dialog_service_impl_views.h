#pragma once

#include "base/win/win_util2.h"
#include "services/dialog_service.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atluser.h>

class DialogServiceImplViews final : public DialogService {
 public:
  // DialogService
  virtual gfx::NativeView GetDialogOwningWindow() const override;
  virtual MessageBoxResult RunMessageBox(base::StringPiece16 message,
                                         base::StringPiece16 title,
                                         MessageBoxMode mode) override;

  gfx::NativeView dialog_owning_window = nullptr;
};

inline MessageBoxResult DialogServiceImplViews::RunMessageBox(
    base::StringPiece16 message,
    base::StringPiece16 title,
    MessageBoxMode mode) {
  const unsigned kFlags[] = {
      MB_ICONINFORMATION | MB_OK,
      MB_ICONSTOP | MB_OK,
      MB_ICONQUESTION | MB_YESNO,
      MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2,
  };
  static_assert(std::size(kFlags) ==
                static_cast<std::size_t>(MessageBoxMode::Count));

  auto title_string = title.as_string();
  if (title_string.empty())
    title_string = win_util::GetWindowText(dialog_owning_window);

  int result = ::AtlMessageBox(
      dialog_owning_window, message.as_string().c_str(), title_string.c_str(),
      kFlags[static_cast<std::size_t>(mode)]);

  switch (result) {
    case IDOK:
      return MessageBoxResult::Ok;
    case IDYES:
      return MessageBoxResult::Yes;
    case IDNO:
      return MessageBoxResult::No;
    case IDCANCEL:
      return MessageBoxResult::Cancel;
    default:
      return MessageBoxResult::Ok;
  }
}

inline gfx::NativeView DialogServiceImplViews::GetDialogOwningWindow() const {
  return dialog_owning_window;
}
