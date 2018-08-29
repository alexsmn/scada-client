#include "components/prompt/prompt_dialog.h"

#include <QInputDialog>

#include "services/dialog_service.h"

bool RunPromptDialog(DialogService& dialog_service,
                     const base::string16& prompt,
                     const base::string16& title,
                     base::string16& value) {
  bool ok = false;
  auto text = QInputDialog::getText(
      dialog_service.GetParentWidget(), QString::fromStdWString(title),
      QString::fromStdWString(prompt), QLineEdit::Normal,
      QString::fromStdWString(value), &ok);
  if (!ok)
    return false;

  value = text.toStdWString();
  return true;
}
