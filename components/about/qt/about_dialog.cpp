#include "components/about/about_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/client_utils_qt.h"
#include "aui/qt/dialog_util.h"
#include "resources/common_resources.h"
#include "project.h"
#include "ui_about_dialog.h"

#include <QApplication>
#include <QMessageBox>

class AboutDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AboutDialog(QWidget* parent = nullptr) : QDialog(parent) {
    ui.setupUi(this);

    ui.icon->setPixmap(LoadPixmap(ID_APPLICATION));

    auto version = tr("Version %1").arg(PROJECT_VERSION_DOTTED_STRING);
    auto organization_name = tr("Telecontrol");
    const int copyright_year = 2018;

    ui.label->setText(
        QString{"<html><head/><body>"
                "<p><b>%1</b></p>"
                "<p>%2</p>"
                "<p>&copy; %3 <a href='http://%4'>%5</a></p>"
                "</body></html>"}
            .arg(QApplication::applicationDisplayName())
            .arg(version)
            .arg(copyright_year)
            .arg(QApplication::organizationDomain())
            .arg(organization_name)
            .arg(PROJECT_VERSION_DOTTED_STRING));
  }

 private:
  Ui::AboutDialog ui;
};

#include "about_dialog.moc"

void ShowAboutDialog(DialogService& dialog_service) {
  auto dialog = std::make_unique<AboutDialog>(dialog_service.GetParentWidget());
  StartModalDialog(std::move(dialog));
}
