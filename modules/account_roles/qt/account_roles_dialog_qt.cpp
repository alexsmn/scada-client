#include "account_roles/qt/account_roles_dialog_qt.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace {

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

}  // namespace

AccountRolesDialog::AccountRolesDialog(
    const std::u16string& account,
    const std::vector<AccountRoleChoice>& choices,
    QWidget* parent)
    : QDialog{parent}, choices_{choices} {
  setObjectName(QStringLiteral("accountRolesDialog"));
  setWindowTitle(Tr("Roles"));

  auto* layout = new QVBoxLayout{this};

  auto* prompt = new QLabel{
      Tr("Roles of user %1. Rights at sign-in come from these roles only.")
          .arg(QString::fromStdU16String(account))};
  prompt->setWordWrap(true);
  layout->addWidget(prompt);

  list_ = new QListWidget;
  list_->setObjectName(QStringLiteral("accountRolesList"));
  for (const AccountRoleChoice& choice : choices_) {
    auto* item = new QListWidgetItem{QString::fromStdU16String(choice.name)};
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    item->setCheckState(choice.held ? Qt::Checked : Qt::Unchecked);
    list_->addItem(item);
  }
  layout->addWidget(list_);

  // A change takes effect at the account's next sign-in, because rights are
  // granted to a session when it is activated (OPC UA Part 18 §4.4.1); saying
  // so stops an administrator from expecting an open session to change.
  auto* note = new QLabel{Tr("The change takes effect at the next sign-in.")};
  note->setWordWrap(true);
  layout->addWidget(note);

  auto* buttons =
      new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel};
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);
}

AccountRolesDialog::~AccountRolesDialog() = default;

std::vector<scada::NodeId> AccountRolesDialog::CheckedRoles() const {
  std::vector<scada::NodeId> checked;
  for (int i = 0; i < list_->count() && i < static_cast<int>(choices_.size());
       ++i) {
    if (list_->item(i)->checkState() == Qt::Checked) {
      checked.push_back(choices_[i].role_id);
    }
  }
  return checked;
}

void ShowAccountRolesDialog(
    DialogService& dialog_service,
    std::u16string account,
    std::vector<AccountRoleChoice> choices,
    std::function<void(std::vector<scada::NodeId> desired)> on_accept) {
  auto* dialog = new AccountRolesDialog{account, choices,
                                        dialog_service.GetParentWidget()};
  dialog->setModal(false);
  QObject::connect(dialog, &QDialog::accepted, dialog,
                   [dialog, on_accept = std::move(on_accept)] {
                     on_accept(dialog->CheckedRoles());
                   });
  QObject::connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
  dialog->show();
}
