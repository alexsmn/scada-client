#include "user_access/qt/users_grid_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <string_view>

namespace {

const scada::aui::ThemeTokens& PanelTokens() {
  return scada::aui::ActiveThemeTokens();
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// The Roles cell. An account with no Role is a plain authenticated user who
// may only look, which is a real state and reads muted; a role set that could
// not be READ says so instead, because rendering it as "no roles" would state
// a fact the client does not have (docs/client/ux/principles.md §5).
QString RolesText(const std::optional<std::vector<AccountRole>>& roles) {
  if (!roles) {
    return Tr("No data");
  }
  if (roles->empty()) {
    return Tr("None");
  }
  QStringList names;
  for (const AccountRole& role : *roles) {
    names << QString::fromStdU16String(role.name);
  }
  return names.join(QStringLiteral(", "));
}

QColor RolesColor(const std::optional<std::vector<AccountRole>>& roles,
                  const scada::aui::ThemeTokens& tokens) {
  if (!roles) {
    return tokens.fg_muted;
  }
  // Holding any Role is authority worth seeing at a glance; holding none is
  // the quiet default.
  return roles->empty() ? tokens.fg_subtle : tokens.fg;
}

// Disabled is the one account state an administrator scans for, so it carries
// the bad token — an account that cannot authenticate is a fact, not an alarm,
// but it must not read as ordinary.
QColor StatusColor(scada::UserConfiguration user_configuration,
                   const scada::aui::ThemeTokens& tokens) {
  return scada::HasUserConfiguration(user_configuration,
                                     scada::UserConfiguration::kDisabled)
             ? tokens.bad
             : tokens.good;
}

}  // namespace

UsersGridPanel::UsersGridPanel(QWidget* parent) : QWidget{parent} {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  setObjectName(QStringLiteral("usersGridPanel"));
  setStyleSheet(
      QStringLiteral("#usersGridPanel{background:%1;}").arg(tokens.bg.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(14, 14, 14, 14);
  root->setSpacing(10);
  root->addWidget(BuildHeader());
  root->addWidget(BuildGrid());
}

UsersGridPanel::~UsersGridPanel() = default;

QWidget* UsersGridPanel::BuildHeader() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* host = new QWidget;
  auto* layout = new QHBoxLayout{host};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  title_ = new QLabel;
  title_->setObjectName(QStringLiteral("usersTitle"));
  title_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));

  add_user_ = new QPushButton{Tr("Add user")};
  reset_password_ = new QPushButton{Tr("Reset password")};
  // Both reuse the host's view context menu. Add-user (parent-scoped create) is
  // always enabled — the create is access-right-gated at the command level, and
  // the admin-role hint explains the requirement. Reset-password acts on the
  // highlighted user, so it enables once a row is selected.
  reset_password_->setEnabled(false);
  connect(add_user_, &QPushButton::clicked, this, [this] {
    Q_EMIT ActionsMenuRequested(
        add_user_->mapToGlobal(add_user_->rect().bottomLeft()),
        /*right_click=*/false);
  });
  connect(reset_password_, &QPushButton::clicked, this, [this] {
    if (HasSelection())
      Q_EMIT ActionsMenuRequested(
          reset_password_->mapToGlobal(reset_password_->rect().bottomLeft()),
          /*right_click=*/false);
  });

  auto* hint = new QLabel{Tr("Editing requires the Administrator role")};
  hint->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_subtle.name()));

  layout->addWidget(title_);
  layout->addStretch(1);
  layout->addWidget(hint);
  layout->addWidget(add_user_);
  layout->addWidget(reset_password_);
  return host;
}

QWidget* UsersGridPanel::BuildGrid() {
  grid_ = new QTableWidget;
  grid_->setObjectName(QStringLiteral("usersGrid"));
  grid_->setColumnCount(4);
  grid_->setHorizontalHeaderLabels(
      {Tr("User"), Tr("Description"), Tr("Roles"), Tr("Status")});
  grid_->verticalHeader()->setVisible(false);
  grid_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  grid_->setSelectionBehavior(QAbstractItemView::SelectRows);
  grid_->setSelectionMode(QAbstractItemView::SingleSelection);
  grid_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  grid_->horizontalHeader()->setStretchLastSection(true);

  grid_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(grid_, &QTableWidget::itemSelectionChanged, this,
          [this] { OnSelectionChanged(); });
  connect(grid_, &QTableWidget::customContextMenuRequested, this,
          [this](const QPoint& pos) { OnContextMenuRequested(pos); });
  return grid_;
}

void UsersGridPanel::ShowRows(const std::vector<UserGridRow>& rows) {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  rows_ = rows;

  title_->setText(QStringLiteral("%1 · %2").arg(Tr("Users")).arg(rows_.size()));

  grid_->setRowCount(static_cast<int>(rows_.size()));
  for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
    const UserGridRow& row = rows_[i];

    auto* name = new QTableWidgetItem(QString::fromStdU16String(row.name));
    grid_->setItem(i, 0, name);

    grid_->setItem(
        i, 1, new QTableWidgetItem(QString::fromStdU16String(row.description)));

    auto* roles = new QTableWidgetItem(RolesText(row.roles));
    roles->setForeground(RolesColor(row.roles, tokens));
    grid_->setItem(i, 2, roles);

    auto* status =
        new QTableWidgetItem(Tr(UserStatusLabelKey(row.user_configuration)));
    status->setForeground(StatusColor(row.user_configuration, tokens));
    grid_->setItem(i, 3, status);
  }
}

bool UsersGridPanel::HasSelection() const {
  return grid_ && grid_->currentRow() >= 0 && !grid_->selectedItems().isEmpty();
}

void UsersGridPanel::OnSelectionChanged() {
  // Reset-password acts on the selected user, so it follows the selection.
  reset_password_->setEnabled(HasSelection());

  const QList<QTableWidgetItem*> selected = grid_->selectedItems();
  if (selected.isEmpty())
    return;
  const int row = selected.front()->row();
  if (row >= 0 && row < static_cast<int>(rows_.size()))
    Q_EMIT UserActivated(rows_[row].node_id);
}

void UsersGridPanel::OnContextMenuRequested(const QPoint& pos) {
  // Select the row under the cursor first, so the selection commands target
  // that user. Emit even over empty space: the CATEGORY_CREATE "New" submenu is
  // parent-scoped and must stay reachable when no user is selected (e.g. an
  // empty Users folder).
  if (QTableWidgetItem* item = grid_->itemAt(pos))
    grid_->selectRow(item->row());
  Q_EMIT ActionsMenuRequested(grid_->viewport()->mapToGlobal(pos),
                              /*right_click=*/true);
}

UsersGridPanel* MakeUsersGridPanel() {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return nullptr;
  return new UsersGridPanel;
}
