#include "modules/about/about_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_util.h"
#include "project.h"
#include "resources/common_resources.h"
#include "ui/qt/client_utils_qt.h"

#include "ui_about_dialog.h"
#include <QIcon>

#include <QApplication>
#include <QMessageBox>

class AboutDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AboutDialog(QWidget* parent = nullptr) : QDialog(parent) {
    ui.setupUi(this);

    // The product's own mark, not a command glyph. It is the one place in the
    // UI that should show what the app *is*, and it is deliberately not routed
    // through LoadPixmap: that path tints to a single palette colour, which
    // would flatten a coloured brand asset (iconography.md §5.4).
    ui.icon->setPixmap(
        QIcon{QStringLiteral(":/icons/app-mark.svg")}.pixmap(64, 64));

    auto version = tr("Version %1").arg(PROJECT_VERSION_DOTTED_STRING);
    // The REGISTERED ENTITY, not the trading name, because the line this
    // feeds is a copyright notice and a notice naming something that is not a
    // legal person asserts nothing. It is the same string as COPYRIGHT and as
    // clause 1 of LICENSE-EXCEPTION.txt, and the three must agree: an operator
    // reading the About box and a lawyer reading the exception have to be
    // looking at the same holder.
    //
    // If the brand name is wanted here instead, this is the one line to change
    // -- but then the About box stops being one of the places the notice is
    // asserted, and COPYRIGHT's "Scope of this notice" has to stop saying it
    // is.
    auto organization_name = tr("Telecontrol, Ltd.");
    // The range this work is claimed over, not the year the dialog was
    // written. It ran as a bare 2018 until 2026-09-20, which understated it at
    // both ends: this repository's first commit is 2017-08-24 and it is still
    // being changed daily. Deliberately NOT derived from the build clock -- an
    // old binary rebuilt next year would then claim a year in which nothing
    // was authored. Widen it by hand when the work is revised, and keep it in
    // step with COPYRIGHT and clause 1 of LICENSE-EXCEPTION.txt.
    const char* copyright_years = "2017\u20132026";

    ui.label->setText(
        QString{"<html><head/><body>"
                "<p><b>%1</b></p>"
                "<p>%2</p>"
                "<p>&copy; %3 <a href='http://%4'>%5</a></p>"
                "</body></html>"}
            .arg(QApplication::applicationDisplayName())
            .arg(version)
            .arg(QString::fromUtf8(copyright_years))
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
  ShowSelfOwnedModalDialog(std::move(dialog));
}
