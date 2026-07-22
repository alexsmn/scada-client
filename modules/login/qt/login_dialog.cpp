#include "modules/login/qt/login_dialog.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
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
  UpdateSecurityVisibility();

  ui.userNameComboBox->view()->setToolTip(
      tr("You can remove the highlighted user from list by pressing Delete."));

  QApplication::instance()->installEventFilter(this);

  // Opt-in reshell chrome, wrapped around the existing form rather than
  // restructuring the .ui: this dialog is the one surface every user must get
  // through, so the legacy layout stays byte-identical when the theme is off.
  if (scada::aui::GetSeverityTheme() != scada::aui::SeverityTheme::kLegacy)
    BuildReshellChrome();

  if (controller_->auto_login) {
    ui.passwordLineEdit->setText(
        QString::fromStdU16String(controller_->password));
    Login();
  }
}

void LoginDialog::BuildReshellChrome() {
  const scada::aui::ThemeTokens& tokens = scada::aui::ActiveThemeTokens();
  auto* root = qobject_cast<QVBoxLayout*>(layout());
  if (!root)
    return;

  // Brand lockup: the mark, the action, and what the operator is signing in to.
  auto* header = new QWidget{this};
  auto* header_layout = new QHBoxLayout{header};
  header_layout->setContentsMargins(0, 0, 0, 8);
  header_layout->setSpacing(10);

  auto* mark = new QLabel{QStringLiteral("TC"), header};
  mark->setObjectName(QStringLiteral("loginBrandMark"));
  mark->setAlignment(Qt::AlignCenter);
  mark->setFixedSize(28, 28);
  mark->setStyleSheet(
      QStringLiteral("#loginBrandMark{background:%1;color:%2;border-radius:6px;"
                     "font-weight:700;}")
          .arg(tokens.accent.name(), tokens.accent_fg.name()));

  auto* titles = new QWidget{header};
  auto* titles_layout = new QVBoxLayout{titles};
  titles_layout->setContentsMargins(0, 0, 0, 0);
  titles_layout->setSpacing(0);
  auto* title = new QLabel{tr("Sign in"), titles};
  title->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));
  auto* subtitle = new QLabel{tr("Telecontrol SCADA operator client"), titles};
  subtitle->setObjectName(QStringLiteral("loginBrandSubtitle"));
  subtitle->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_subtle.name()));
  titles_layout->addWidget(title);
  titles_layout->addWidget(subtitle);

  header_layout->addWidget(mark);
  header_layout->addWidget(titles);
  header_layout->addStretch(1);
  root->insertWidget(0, header);

  // "You are connecting to" — the wrong-server guard. Only the backend and
  // server are shown because they are all this dialog knows before it
  // authenticates; see LoginConnectionSummary.
  connection_summary_ = new QLabel{this};
  connection_summary_->setObjectName(QStringLiteral("loginConnectionSummary"));
  connection_summary_->setWordWrap(true);
  connection_summary_->setStyleSheet(
      QStringLiteral("#loginConnectionSummary{background:%1;color:%2;"
                     "border:1px solid %3;border-radius:6px;padding:6px 9px;"
                     "font-size:11px;}")
          .arg(tokens.surface_muted.name(), tokens.fg_muted.name(),
               tokens.border.name(QColor::HexArgb)));
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

  // The backend combo is hidden when only one backend is built in; its text is
  // still the honest name of what will be connected to.
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
  ui.securityModeComboBox->setEnabled(enable);
  ui.certificateLineEdit->setEnabled(enable);
  ui.certificateBrowseButton->setEnabled(enable);
  ui.privateKeyLineEdit->setEnabled(enable);
  ui.privateKeyBrowseButton->setEnabled(enable);
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void LoginDialog::UpdateSecurityVisibility() {
  const bool show = controller_->IsSecuritySupported();
  ui.securityLabel->setVisible(show);
  ui.securityModeComboBox->setVisible(show);
  ui.certificateLabel->setVisible(show);
  ui.certificateWidget->setVisible(show);
  ui.privateKeyLabel->setVisible(show);
  ui.privateKeyWidget->setVisible(show);
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
