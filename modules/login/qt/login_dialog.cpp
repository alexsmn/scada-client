#include "modules/login/qt/login_dialog.h"

#include "base/e2e_test_hooks.h"
#include "modules/login/login_controller.h"
#include "modules/login/login_summary.h"
#include "net/net_executor_adapter.h"
#include "scada/session_service.h"
#include "scada/status.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QFileDialog>
#include <QKeyEvent>
#include <QSettings>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qdialogbuttonbox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qtoolbutton.h>

namespace {

QStringList MakeQStringList(const std::vector<std::u16string>& source) {
  QStringList list;
  list.reserve(source.size());
  for (auto& str : source)
    list.push_back(QString::fromStdU16String(str));
  return list;
}

Awaitable<std::optional<DataServices>> DeleteLoginDialogOnCompletionAsync(
    LoginDialog& login_dialog) {
  auto result = co_await login_dialog.Wait();
  login_dialog.deleteLater();
  co_return result;
}

}  // namespace

LoginDialog::LoginDialog(AnyExecutor executor,
                         DataServicesContext&& services_context,
                         std::shared_ptr<SettingsStore> settings_store)
    : controller_{std::make_shared<LoginController>(
          executor,
          std::move(services_context),
          dialog_service_,
          settings_store ? std::move(settings_store)
                         : client::CreateE2eSettingsStore())},
      completion_{std::move(executor)} {
  ui.setupUi(this);

  // Name the action rather than the assent (docs/client/ux/dialogs.md §3). The
  // heading that used to say this lived in the content area over the real
  // title bar and has been removed, so the button carries the verb.
  ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Sign in"));

  dialog_service_.parent_widget = this;

  controller_->completion_handler = [this](DataServices services) {
    Complete(std::move(services));
  };

  controller_->login_failed_handler = [this](const scada::Status& status) {
    if (!client::IsE2eTestMode())
      return false;

    client::ReportE2eStatus(std::string{"failure: "} + ToString(status));
    Complete(std::nullopt);
    close();
    return true;
  };

  controller_->error_handler = [this] {
    EnableControls(true);
    ui.userNameComboBox->setFocus();
    ui.userNameComboBox->lineEdit()->selectAll();
  };

  ui.serverTypeComboBox->addItems(
      MakeQStringList(controller_->server_type_list));
  ui.serverTypeComboBox->setCurrentIndex(controller_->server_type_index());
  ui.serverTypeComboBox->setVisible(controller_->server_type_list.size() >= 2);
  connect(ui.serverTypeComboBox,
          qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int server_type_index) {
            controller_->server_host =
                ui.serverComboBox->currentText().toStdString();
            controller_->SetServerTypeIndex(server_type_index);
            ui.serverComboBox->setCurrentText(
                QString::fromStdString(controller_->server_host));
            UpdateSecurityVisibility();
          });

  ui.serverComboBox->setCurrentText(
      QString::fromStdString(controller_->server_host));

  ui.userNameComboBox->addItems(MakeQStringList(controller_->user_list));
  ui.userNameComboBox->setCurrentText(
      QString::fromStdU16String(controller_->user_name));
  ui.userNameComboBox->lineEdit()->selectAll();

  ui.autoLoginCheckBox->setChecked(controller_->auto_login);

  ui.securityModeComboBox->addItems(
      MakeQStringList(controller_->security_mode_list));
  ui.securityModeComboBox->setCurrentIndex(controller_->security_mode_index);
  ui.certificateLineEdit->setText(
      QString::fromStdString(controller_->client_certificate_path));
  ui.privateKeyLineEdit->setText(
      QString::fromStdString(controller_->client_private_key_path));
  connect(ui.certificateBrowseButton, &QPushButton::clicked, this, [this] {
    BrowseForFile(*ui.certificateLineEdit, tr("Select client certificate"));
  });
  connect(ui.privateKeyBrowseButton, &QPushButton::clicked, this, [this] {
    BrowseForFile(*ui.privateKeyLineEdit, tr("Select client private key"));
  });
  // The three OPC UA security rows sit behind a disclosure, collapsed.
  // `login.html` draws none of them: they are configuration an operator sets
  // once and a deployment often never, where everything else on this form is
  // supplied at every sign-in. Expanded when any of them already carries a
  // value, so an operator who HAS configured a certificate is not asked to go
  // looking for it.
  ui.securityToggleButton->setChecked(
      !controller_->client_certificate_path.empty() ||
      !controller_->client_private_key_path.empty() ||
      controller_->security_mode_index > 0);
  connect(ui.securityToggleButton, &QToolButton::toggled, this,
          [this] { UpdateSecurityVisibility(); });
  UpdateSecurityVisibility();

  ui.userNameComboBox->view()->setToolTip(
      tr("You can remove the highlighted user from list by pressing Delete."));

  QApplication::instance()->installEventFilter(this);

  // Workbench chrome, wrapped around the existing form rather than
  // restructuring the .ui: this dialog is the one surface every user must get
  // through, so the form itself stays untouched.
  BuildReshellChrome();

  if (controller_->auto_login) {
    ui.passwordLineEdit->setText(
        QString::fromStdU16String(controller_->password));
    Login();
  }
}

void LoginDialog::BuildReshellChrome() {
  auto* root = qobject_cast<QVBoxLayout*>(layout());
  if (!root)
    return;

  // No heading, no brand lockup: the window already has a real title bar
  // saying "Login", and the application is identified by the window itself
  // (docs/client/ux/dialogs.md §1). Drawing them again is a browser-modal habit
  // — a modal in a page has no OS chrome and must supply its own; a QDialog
  // does not, and repeating it cost a third of the dialog's height before the
  // first field.

  // "You are connecting to" — the wrong-server guard, now a quiet line rather
  // than a bordered card.
  //
  // Two things changed here and both are about the same defect. It was a
  // panel — background, border, radius, 6px padding, a baked 11px font — laid
  // on top of a form whose every other row is drawn by the platform style, so
  // the largest block on this dialog was the one carrying the least. And all
  // of that came from a `setStyleSheet` with colours baked out of the theme
  // tokens, which the native-look rules call a regression to remove rather
  // than add (client/CLAUDE.md, "Colour through QPalette roles"). It is now a
  // label quietened through the palette, which follows the platform's own
  // light/dark switch with nothing to keep in step.
  connection_summary_ = new QLabel{this};
  connection_summary_->setWordWrap(true);
  QPalette quiet = connection_summary_->palette();
  quiet.setColor(connection_summary_->foregroundRole(),
                 quiet.color(QPalette::PlaceholderText));
  connection_summary_->setPalette(quiet);
  root->insertWidget(root->count() - 1, connection_summary_);

  // The summary tracks whichever field the operator edits.
  const auto refresh = [this] { RefreshConnectionSummary(); };
  connect(ui.serverComboBox, &QComboBox::currentTextChanged, this, refresh);
  connect(ui.serverTypeComboBox, &QComboBox::currentTextChanged, this, refresh);
  RefreshConnectionSummary();
}

void LoginDialog::RefreshConnectionSummary() {
  if (!connection_summary_)
    return;

  // **Shown only when it says something the form above does not.** The address
  // is in `serverComboBox` on every single render, so restating it underneath
  // is one of the two controls this dialog has, said twice. What the summary
  // can carry alone is the BACKEND name, and only in the build where
  // `serverTypeComboBox` is hidden because one backend is compiled in — there
  // the operator has no other way to see which protocol they are about to
  // speak. Where both controls are visible the line is pure duplication and is
  // not drawn, which is what makes the steady-state dialog three rows and a
  // button (`docs/product/ui-mockups/screens/login.html`).
  //
  // `isHidden()`, not `isVisible()`: this runs from the constructor, before the
  // window is shown, and `isVisible()` is false for every widget in a window
  // that has not been shown yet — so asking it here hid nothing and drew the
  // duplicate line anyway. `isHidden()` answers the question actually being
  // asked, which is whether the combo was explicitly hidden a few lines above.
  if (!ui.serverTypeComboBox->isHidden()) {
    connection_summary_->setVisible(false);
    return;
  }

  const std::u16string summary = LoginConnectionSummary(
      ui.serverTypeComboBox->currentText().toStdU16String(),
      ui.serverComboBox->currentText().toStdU16String());
  connection_summary_->setVisible(!summary.empty());
  if (summary.empty())
    return;

  connection_summary_->setText(
      tr("Connecting to: %1").arg(QString::fromStdU16String(summary)));
}

LoginDialog::~LoginDialog() {
  QApplication::instance()->removeEventFilter(this);
}

void LoginDialog::accept() {
  Login();
}

void LoginDialog::reject() {
  if (client::IsE2eTestMode()) {
    client::ReportE2eStatusIfUnset("canceled");
  }

  // Close the dialog *before* completing. Completing resumes the startup
  // coroutine, which on cancel throws LoginCanceled and ends in
  // QCoreApplication::quit() (client::RunQtStartupFlow). On macOS a visible
  // modal dialog keeps QCocoaEventDispatcher inside an AppKit modal session
  // (-[NSApplication runModalSession:]), and a quit issued from inside that
  // session is lost when the session ends: the process drops back into the
  // main event loop with no window and never exits. Hiding first ends the
  // modal session, so the quit reaches the loop that is actually running.
  QDialog::reject();

  Complete(std::nullopt);
}

Awaitable<std::optional<DataServices>> LoginDialog::Wait() {
  co_await completion_.Wait();
  co_return std::move(result_);
}

void LoginDialog::Complete(std::optional<DataServices> services) {
  if (completed_) {
    return;
  }
  completed_ = true;
  result_ = std::move(services);
  completion_.Complete();
}

void LoginDialog::Login() {
  EnableControls(false);

  controller_->SetServerTypeIndex(controller_->server_type_list.size() >= 2
                                      ? ui.serverTypeComboBox->currentIndex()
                                      : 0);
  controller_->server_host = ui.serverComboBox->currentText().toStdString();
  controller_->user_name = ui.userNameComboBox->currentText().toStdU16String();
  controller_->password = ui.passwordLineEdit->text().toStdU16String();
  controller_->auto_login = ui.autoLoginCheckBox->isChecked();
  controller_->security_mode_index = ui.securityModeComboBox->currentIndex();
  controller_->client_certificate_path =
      ui.certificateLineEdit->text().toStdString();
  controller_->client_private_key_path =
      ui.privateKeyLineEdit->text().toStdString();

  controller_->Login();
}

void LoginDialog::EnableControls(bool enable) {
  ui.serverTypeComboBox->setEnabled(enable);
  ui.serverComboBox->setEnabled(enable);
  ui.userNameComboBox->setEnabled(enable);
  ui.passwordLineEdit->setEnabled(enable);
  ui.autoLoginCheckBox->setEnabled(enable);
  ui.securityToggleButton->setEnabled(enable);
  ui.securityModeComboBox->setEnabled(enable);
  ui.certificateLineEdit->setEnabled(enable);
  ui.certificateBrowseButton->setEnabled(enable);
  ui.privateKeyLineEdit->setEnabled(enable);
  ui.privateKeyBrowseButton->setEnabled(enable);
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void LoginDialog::UpdateSecurityVisibility() {
  // Two conditions, and they mean different things. A backend that has no
  // notion of endpoint security hides the disclosure itself — the rows are not
  // collapsed there, they do not exist. A backend that does shows the
  // disclosure and lets the operator decide whether to look.
  const bool supported = controller_->IsSecuritySupported();
  ui.securityToggleButton->setVisible(supported);
  ui.securityToggleButton->setArrowType(
      ui.securityToggleButton->isChecked() ? Qt::DownArrow : Qt::RightArrow);

  const bool show = supported && ui.securityToggleButton->isChecked();
  ui.securityLabel->setVisible(show);
  ui.securityModeComboBox->setVisible(show);
  ui.certificateLabel->setVisible(show);
  ui.certificateWidget->setVisible(show);
  ui.privateKeyLabel->setVisible(show);
  ui.privateKeyWidget->setVisible(show);

  // The window is sized to its content and has no scroll area, so collapsing
  // the section has to give the height back rather than leaving a gap.
  adjustSize();
}

void LoginDialog::BrowseForFile(QLineEdit& target, const QString& title) {
  const QString path = QFileDialog::getOpenFileName(
      this, title, target.text(), tr("PEM files (*.pem);;All files (*)"));
  if (!path.isEmpty())
    target.setText(path);
}

bool LoginDialog::eventFilter(QObject* object, QEvent* event) {
  if (object == ui.userNameComboBox->view()) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
      if (key_event->key() == Qt::Key_Delete) {
        int index = ui.userNameComboBox->view()->currentIndex().row();
        if (index != -1) {
          controller_->DeleteUserName(
              ui.userNameComboBox->itemText(index).toStdU16String());
          ui.userNameComboBox->removeItem(index);
        }
        return true;
      }
    }
  }

  return QDialog::eventFilter(object, event);
}

Awaitable<std::optional<DataServices>> ExecuteLoginDialog(
    AnyExecutor executor,
    DataServicesContext services_context,
    std::shared_ptr<SettingsStore> settings_store) {
  LoginDialog* login_dialog =
      new LoginDialog{std::move(executor), std::move(services_context),
                      std::move(settings_store)};

  login_dialog->setModal(true);
  login_dialog->show();
  co_return co_await DeleteLoginDialogOnCompletionAsync(*login_dialog);
}
