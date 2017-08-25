#include "client/client_utils.h"

#include <QMessageBox>

#include "client/components/main/qt/main_window_qt.h"

int ShowMessageBox(DialogService& dialog_service, const base::char16* message,
                   const base::char16* title, unsigned types) {
  auto button_type = types & 0xf;
  auto buttons = (button_type == MB_YESNO) ? QMessageBox::Yes | QMessageBox::No :
                 (button_type == MB_OKCANCEL) ? QMessageBox::Ok | QMessageBox::Cancel :
                 QMessageBox::Ok;

  auto icon_type = types & 0xf0;
  auto icon = (icon_type == MB_ICONINFORMATION) ? QMessageBox::Information :
              (icon_type == MB_ICONEXCLAMATION) ? QMessageBox::Warning :
              (icon_type == MB_ICONERROR) ? QMessageBox::Critical :
              (icon_type == MB_ICONQUESTION) ? QMessageBox::Question :
              QMessageBox::NoIcon;

  auto* message_box = new QMessageBox(static_cast<DialogServiceQt&>(dialog_service).GetParentView());
  message_box->setStandardButtons(buttons);
  message_box->setIcon(icon);
  message_box->setText(QString::fromWCharArray(message));
  message_box->setWindowTitle(QString::fromWCharArray(title));

  auto button = message_box->exec();

  switch (button) {
    case QMessageBox::Yes:
      return IDYES;
    case QMessageBox::No:
      return IDNO;
    case QMessageBox::Ok:
      return IDOK;
    case QMessageBox::Cancel:
      return IDCANCEL;
    default:
      return IDOK;
  }
}
