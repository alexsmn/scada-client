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
  scada::aui::Theme theme = scada::aui::Theme::kDark;
  switch (scada::aui::GetSeverityTheme()) {
    case scada::aui::SeverityTheme::kLight:
      theme = scada::aui::Theme::kLight;
      break;
    case scada::aui::SeverityTheme::kHighContrast:
      theme = scada::aui::Theme::kHighContrast;
      break;
    default:
      break;
  }
  return scada::aui::GetThemeTokens(theme);
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// The role cell's colour, matching the RBAC inspector's pill: Administrator
// reads bad (asserted authority, not an alarm), Operator good, Observer muted.
QColor RoleColor(UserRole role, const scada::aui::ThemeTokens& tokens) {
  switch (role) {
    case UserRole::kAdministrator:
      return tokens.bad;
    case UserRole::kOperator:
      return tokens.good;
    case UserRole::kObserver:
      break;
  }
  return tokens.fg_subtle;
}

}  // namespace

UsersGridPanel::UsersGridPanel(QWidget* parent) : QWidget{parent} {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  setObjectName(QStringLiteral("usersGridPanel"));
  setStyleSheet(QStringLiteral("#usersGridPanel{background:%1;}")
                    .arg(tokens.bg.name()));

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
  // The create/reset write path is not wired at this surface; the affordances
  // are shown disabled to match the mockup and its admin-gating hint.
  add_user_->setEnabled(false);
  reset_password_->setEnabled(false);

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
  grid_->setColumnCount(3);
  grid_->setHorizontalHeaderLabels({Tr("User"), Tr("Role"), Tr("Sessions")});
  grid_->verticalHeader()->setVisible(false);
  grid_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  grid_->setSelectionBehavior(QAbstractItemView::SelectRows);
  grid_->setSelectionMode(QAbstractItemView::SingleSelection);
  grid_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  grid_->horizontalHeader()->setStretchLastSection(true);

  connect(grid_, &QTableWidget::itemSelectionChanged, this,
          [this] { OnSelectionChanged(); });
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

    auto* role = new QTableWidgetItem(Tr(UserRoleLabelKey(row.role)));
    role->setForeground(RoleColor(row.role, tokens));
    grid_->setItem(i, 1, role);

    auto* sessions =
        new QTableWidgetItem(Tr(UserSessionsLabelKey(row.multi_sessions)));
    grid_->setItem(i, 2, sessions);
  }
}

void UsersGridPanel::OnSelectionChanged() {
  const QList<QTableWidgetItem*> selected = grid_->selectedItems();
  if (selected.isEmpty())
    return;
  const int row = selected.front()->row();
  if (row >= 0 && row < static_cast<int>(rows_.size()))
    Q_EMIT UserActivated(rows_[row].node_id);
}

UsersGridPanel* MakeUsersGridPanel() {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return nullptr;
  return new UsersGridPanel;
}
