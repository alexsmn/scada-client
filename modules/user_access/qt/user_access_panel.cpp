#include "user_access/qt/user_access_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
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

// The permission rows for the Roles an account holds, already translated.
std::vector<UserPermissionDisplay> MakePermissionDisplays(
    std::span<const AccountRole> roles) {
  std::vector<UserPermissionDisplay> displays;
  for (const UserPermission& permission : PermissionsForRoles(roles)) {
    displays.push_back(UserPermissionDisplay{
        Tr(UserPermissionLabelKey(permission.kind)), permission.granted});
  }
  return displays;
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

  // Header: account name, then the Roles it holds. A list rather than one
  // pill: an account can hold several Roles, and collapsing them into a single
  // tier is exactly the fiction the access-rights bitmask used to tell.
  auto* header = new QHBoxLayout;
  name_ = new QLabel;
  name_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));
  header->addWidget(name_);
  header->addStretch(1);
  layout->addLayout(header);

  auto* roles_header = new QLabel{Tr("Roles")};
  roles_header->setStyleSheet(
      QStringLiteral("color:%1;font-size:10px;font-weight:600;"
                     "text-transform:uppercase;letter-spacing:.5px;")
          .arg(tokens.fg_subtle.name()));
  layout->addWidget(roles_header);

  roles_ = new QLabel;
  roles_->setObjectName(QStringLiteral("userRoles"));
  roles_->setWordWrap(true);
  layout->addWidget(roles_);

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
  pending_name_.clear();
  if (stack_)
    stack_->setCurrentIndex(0);
}

void UserAccessPanel::ShowUser(const NodeRef& user,
                               NodeService& node_service,
                               scada::AttributeService& attribute_service,
                               AnyExecutor executor) {
  if (!user || !IsInstanceOf(user, scada::security::id::UserType)) {
    Clear();
    return;
  }

  const QString name =
      QString::fromStdU16String(ToString16(user.display_name()));
  pending_name_ = name;

  // Show the account at once with its Roles unresolved, then fill them. The
  // shell's selection handler is synchronous, and the RoleSet needs a browse.
  ShowAccount(name, std::nullopt);

  CoSpawn(executor,
          [this, name, &node_service, &attribute_service, executor,
           token = std::weak_ptr<int>{lifetime_token_}]() -> Awaitable<void> {
            auto roles = co_await ReadRoleMemberships(executor, node_service,
                                                      attribute_service);
            // Dropped if the panel died, or if a newer selection has since
            // replaced this one — a stale fill would attribute one account's
            // Roles to another.
            if (token.expired() || pending_name_ != name) {
              co_return;
            }
            std::optional<std::vector<AccountRole>> account_roles;
            if (roles) {
              auto by_account = RolesByAccount(*roles);
              auto i = by_account.find(name.toStdU16String());
              account_roles = i != by_account.end()
                                  ? i->second
                                  : std::vector<AccountRole>{};
            }
            ShowAccount(name, account_roles);
            co_return;
          });
}

void UserAccessPanel::ShowAccount(
    const QString& name,
    const std::optional<std::vector<AccountRole>>& roles) {
  if (name.isEmpty()) {
    Clear();
    return;
  }

  // An unreadable RoleSet says nothing about the account's Roles, and
  // therefore nothing about its permissions either — so neither is drawn.
  // Rendering "no roles" or a row of denied permissions would both be claims
  // the client cannot support (docs/client/ux/principles.md §5).
  if (!roles) {
    ShowAccess(name, {}, {});
    return;
  }

  QStringList role_names;
  for (const AccountRole& role : *roles) {
    role_names << QString::fromStdU16String(role.name);
  }
  ShowAccess(name, role_names, MakePermissionDisplays(*roles));
}

void UserAccessPanel::ShowAccess(
    const QString& name,
    const QStringList& roles,
    const std::vector<UserPermissionDisplay>& permissions) {
  const scada::aui::ThemeTokens& tokens = PanelTokens();

  name_->setText(name);

  // Three distinct states, and they must not be conflated: unknown (nothing
  // was read), none (read, and the account holds no Role — a real and
  // ordinary state), and the list itself.
  if (roles.isEmpty()) {
    roles_->setText(permissions.empty() ? Tr("No data") : Tr("None"));
    roles_->setStyleSheet(
        QStringLiteral("color:%1;").arg(tokens.fg_subtle.name()));
  } else {
    roles_->setText(roles.join(QStringLiteral(", ")));
    roles_->setStyleSheet(QStringLiteral("color:%1;").arg(tokens.fg.name()));
  }

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
  return new UserAccessPanel;
}
