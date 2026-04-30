#include "aui/prompt_dialog.h"

#include "aui/wt/dialog_stub.h"

Awaitable<std::u16string> RunPromptDialog(DialogService& dialog_service,
                                          const std::u16string& prompt,
                                          const std::u16string& title,
                                          const std::u16string& initial_value) {
  return aui::wt::MakeUnsupportedDialogAwaitable<std::u16string>();
}
