#include "aui/prompt_dialog.h"

promise<std::u16string> RunPromptDialog(DialogService& dialog_service,
                                        const std::u16string& prompt,
                                        const std::u16string& title,
                                        const std::u16string& initial_value) {
  return make_rejected_promise<std::u16string>(std::exception{});
}
