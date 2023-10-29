#include "components/prompt/prompt_dialog.h"

#include "qt/dialog_util.h"
#include "aui/dialog_service.h"

#include <QInputDialog>

promise<std::u16string> RunPromptDialog(DialogService& dialog_service,
                                        const std::u16string& prompt,
                                        const std::u16string& title,
                                        const std::u16string& initial_value) {
  auto dialog =
      std::make_unique<QInputDialog>(dialog_service.GetParentWidget());
  dialog->setWindowTitle(QString::fromStdU16String(title));
  dialog->setLabelText(QString::fromStdU16String(prompt));
  dialog->setTextValue(QString::fromStdU16String(initial_value));
  return StartModalDialog(std::move(dialog)).then([](QInputDialog* dialog) {
    return dialog->textValue().toStdU16String();
  });
}
