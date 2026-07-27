#include "user_access/qt/user_access_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "scada/basic_types.h"
#include "scada/variant.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <string_view>
#include <utility>

namespace {

const scada::aui::ThemeTokens& PanelTokens() {
  return scada::aui::ActiveThemeTokens();
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// The role pill's colour, matching users-admin.html: Administrator reads bad
// (asserted authority, not an alarm), Operator good, Observer muted. kUnknown
// is deliberately muted rather than alarming — nothing is wrong with the user,
// the client simply has not read their rights yet — but it must not borrow any
// real role's colour.
QColor RolePillColor(UserRole role, const scada::aui::ThemeTokens& tokens) {
  switch (role) {
    case UserRole::kAdministrator:
      return tokens.bad;
    case UserRole::kOperator:
      return tokens.good;
    case UserRole::kUnknown:
      return tokens.fg_muted;
    case UserRole::kObserver:
      break;
  }
  return tokens.fg_subtle;
}

}  // namespace

UserAccessPanel::UserAccessPanel(QWidget* parent) : QWidget{parent} {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  setObjectName(QStringLiteral("userAccessPanel"));
  setStyleSheet(QStringLiteral("#userAccessPanel{background:%1;}")
                    .arg(tokens.bg_elevated.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(0, 0, 0, 0);

  stack_ = new QStackedWidget{this};
  stack_->addWidget(BuildEmptyState());  // index 0
  stack_->addWidget(BuildContent());     // index 1
  root->addWidget(stack_);

  Clear();
}

UserAccessPanel::~UserAccessPanel() = default;

QWidget* UserAccessPanel::BuildEmptyState() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* empty = new QWidget;
  auto* layout = new QVBoxLayout{empty};
  layout->setAlignment(Qt::AlignCenter);
  auto* label = new QLabel{Tr("Select a user to see its access rights")};
  label->setWordWrap(true);
  label->setAlignment(Qt::AlignCenter);
  label->setStyleSheet(
      QStringLiteral("color:%1;padding:24px;").arg(tokens.fg_subtle.name()));
  layout->addWidget(label);
  return empty;
}

QWidget* UserAccessPanel::BuildContent() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* view = new QWidget;
  auto* layout = new QVBoxLayout{view};
  layout->setContentsMargins(14, 14, 14, 14);
  layout->setSpacing(12);

  // Header: user name + role pill.
  auto* header = new QHBoxLayout;
  name_ = new QLabel;
  name_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));
  role_ = new QLabel;
  role_->setObjectName(QStringLiteral("userRolePill"));
  header->addWidget(name_);
  header->addStretch(1);
  header->addWidget(role_);
  layout->addLayout(header);

  // Permissions section (rows rebuilt per user).
  auto* perms_header = new QLabel{Tr("Permissions")};
  perms_header->setStyleSheet(
      QStringLiteral("color:%1;font-size:10px;font-weight:600;"
                     "text-transform:uppercase;letter-spacing:.5px;")
          .arg(tokens.fg_subtle.name()));
  layout->addWidget(perms_header);

  auto* perms_host = new QWidget;
  perms_layout_ = new QVBoxLayout{perms_host};
  perms_layout_->setContentsMargins(0, 0, 0, 0);
  perms_layout_->setSpacing(0);
  layout->addWidget(perms_host);

  layout->addStretch(1);
  return view;
}

void UserAccessPanel::Clear() {
  if (stack_)
    stack_->setCurrentIndex(0);
}

void UserAccessPanel::ShowUser(const NodeRef& user) {
  if (!user || !IsInstanceOf(user, scada::security::id::UserType)) {
    Clear();
    return;
  }

  const QString name =
      QString::fromStdU16String(ToString16(user.display_name()));

  // This is a synchronous selection handler (see MainWindowQt's selection
  // routing): it renders whatever is already resident and never fetches, so
  // AccessRights may not have been read yet. Variant::get() is the honest
  // predicate — it fails both for a Variant that was never delivered and for
  // one holding an unreadable type — whereas get_or(0) collapses either into a
  // zero bitmask, which is indistinguishable from a genuine Observer with only
  // View granted. Presenting an unresolved read as a real role is exactly the
  // failure mode docs/ux/principles.md §5 forbids.
  scada::Int32 access = 0;
  if (!user[scada::security::id::UserType_AccessRights].value().get(access)) {
    // No permission rows either: an unresolved bitmask says nothing about the
    // individual permissions, so drawing them ungranted would be just as false
    // a claim as drawing them granted. ShowAccess renders an explicit
    // placeholder for the empty list.
    ShowAccess(name, UserRole::kUnknown, {});
    return;
  }

  std::vector<UserPermissionDisplay> permissions;
  for (const UserPermission& permission : UserPermissionsFor(access)) {
    permissions.push_back(UserPermissionDisplay{
        Tr(UserPermissionLabelKey(permission.kind)), permission.granted});
  }

  ShowAccess(name, UserRoleFor(access), permissions);
}

void UserAccessPanel::ShowAccess(
    const QString& name,
    UserRole role,
    const std::vector<UserPermissionDisplay>& permissions) {
  const scada::aui::ThemeTokens& tokens = PanelTokens();

  name_->setText(name);

  const QColor pill = RolePillColor(role, tokens);
  role_->setText(Tr(UserRoleLabelKey(role)));
  role_->setStyleSheet(
      QStringLiteral("#userRolePill{color:%1;border:1px solid %1;"
                     "border-radius:9px;padding:1px 10px;font-weight:600;}")
          .arg(pill.name()));

  // Rebuild the permission rows.
  while (QLayoutItem* item = perms_layout_->takeAt(0)) {
    if (QWidget* widget = item->widget())
      widget->deleteLater();
    delete item;
  }
  // An empty breakdown means the rights could not be read. Say so with the
  // Inspector's em-dash placeholder rather than leaving a bare section header,
  // which reads as a rendering fault.
  if (permissions.empty()) {
    auto* placeholder = new QLabel{QStringLiteral("—")};
    placeholder->setObjectName(QStringLiteral("userPermissionsPlaceholder"));
    placeholder->setStyleSheet(
        QStringLiteral("color:%1;padding:4px 0;").arg(tokens.fg_subtle.name()));
    perms_layout_->addWidget(placeholder);
  }
  for (const UserPermissionDisplay& permission : permissions) {
    auto* row = new QWidget;
    auto* row_layout = new QHBoxLayout{row};
    row_layout->setContentsMargins(0, 4, 0, 4);
    row_layout->setSpacing(8);

    // A small check box: filled accent when granted, hollow otherwise.
    auto* check = new QLabel;
    check->setFixedSize(14, 14);
    check->setStyleSheet(
        permission.granted
            ? QStringLiteral("background:%1;border-radius:4px;")
                  .arg(tokens.accent.name())
            : QStringLiteral("border:1px solid %1;border-radius:4px;")
                  .arg(tokens.border_strong.name()));

    auto* label = new QLabel{permission.label};
    label->setStyleSheet(
        QStringLiteral("color:%1;")
            .arg((permission.granted ? tokens.fg : tokens.fg_subtle).name()));

    row_layout->addWidget(check);
    row_layout->addWidget(label);
    row_layout->addStretch(1);
    perms_layout_->addWidget(row);
  }

  stack_->setCurrentIndex(1);
}

UserAccessPanel* MakeUserAccessPanel() {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return nullptr;
  return new UserAccessPanel;
}
