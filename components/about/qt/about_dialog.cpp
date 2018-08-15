#include "components/about/about_dialog.h"

#include <QApplication>
#include <QMessageBox>

#include "project.h"
#include "services/dialog_service.h"

void ShowAboutDialog(DialogService& dialog_service) {
  auto text = QApplication::applicationDisplayName() + ' ' +
              PROJECT_VERSION_DOTTED_STRING;

  QMessageBox::about(dialog_service.GetParentWidget(), QObject::tr("About"),
                     text);
}
