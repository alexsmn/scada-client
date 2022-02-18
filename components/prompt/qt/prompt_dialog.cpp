#include "components/prompt/prompt_dialog.h"

#include <QInputDialog>

#include "services/dialog_service.h"

bool RunPromptDialog(DialogService& dialog_service,
                     const std::u16string& prompt,
                     const std::u16string& title,
                     std::u16string& value) {
  bool ok = false;
  auto text = QInputDialog::getText(
      dialog_service.GetParentWidget(), QString::fromStdU16String(title),
      QString::fromStdU16String(prompt), QLineEdit::Normal,
      QString::fromStdU16String(value), &ok);
  if (!ok)
    return false;

  value = text.toStdU16String();
  return true;
}
