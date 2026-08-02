#include "user_access/qt/roles_grid_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QStringList>
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

// A Role nobody holds is a real and unremarkable state — an empty Role is how
// one starts — so it reads quiet rather than as a warning.
QString MembersText(const std::vector<std::u16string>& members) {
  if (members.empty()) {
    return Tr("None");
  }
  QStringList names;
  for (const std::u16string& member : members) {
    names << QString::fromStdU16String(member);
  }
  return names.join(QStringLiteral(", "));
}

}  // namespace

RolesGridPanel::RolesGridPanel(QWidget* parent) : QWidget{parent} {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  setObjectName(QStringLiteral("rolesGridPanel"));
  setStyleSheet(
      QStringLiteral("#rolesGridPanel{background:%1;}").arg(tokens.bg.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(12, 10, 12, 12);
  root->setSpacing(8);
  root->addWidget(BuildHeader());
  root->addWidget(BuildGrid());
}

RolesGridPanel::~RolesGridPanel() = default;

QWidget* RolesGridPanel::BuildHeader() {
  const scada::aui::ThemeTokens& tokens = PanelTokens();
  auto* host = new QWidget;
  auto* layout = new QHBoxLayout{host};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  title_ = new QLabel;
  title_->setObjectName(QStringLiteral("rolesTitle"));
  title_->setStyleSheet(
      QStringLiteral("color:%1;font-size:14px;font-weight:600;")
          .arg(tokens.fg.name()));

  hint_ = new QLabel{Tr("Editing requires the Administrator role")};
  hint_->setObjectName(QStringLiteral("rolesHint"));
  hint_->setStyleSheet(
      QStringLiteral("color:%1;font-size:11px;").arg(tokens.fg_subtle.name()));

  layout->addWidget(title_);
  layout->addStretch(1);
  layout->addWidget(hint_);
  return host;
}

QWidget* RolesGridPanel::BuildGrid() {
  grid_ = new QTableWidget;
  grid_->setObjectName(QStringLiteral("rolesGrid"));
  grid_->setColumnCount(3);
  grid_->setHorizontalHeaderLabels({Tr("Role"), Tr("Members"), Tr("Type")});
  grid_->verticalHeader()->setVisible(false);
  grid_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  grid_->setSelectionBehavior(QAbstractItemView::SelectRows);
  grid_->setSelectionMode(QAbstractItemView::SingleSelection);
  grid_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  return grid_;
}

void RolesGridPanel::ShowRoles(
    const std::optional<std::vector<RoleMembership>>& roles) {
  const scada::aui::ThemeTokens& tokens = PanelTokens();

  // A RoleSet that could not be read is said so, not drawn as a server with no
  // Roles — a conformant server always publishes the well-known ones, so an
  // empty list would be a fact the client does not have.
  if (!roles) {
    roles_.clear();
    title_->setText(QStringLiteral("%1 · %2").arg(Tr("Roles"), Tr("No data")));
    grid_->setRowCount(0);
    return;
  }

  roles_ = *roles;
  title_->setText(QStringLiteral("%1 · %2").arg(Tr("Roles")).arg(roles_.size()));

  grid_->setRowCount(static_cast<int>(roles_.size()));
  for (int i = 0; i < static_cast<int>(roles_.size()); ++i) {
    const RoleMembership& role = roles_[i];

    grid_->setItem(
        i, 0, new QTableWidgetItem(QString::fromStdU16String(role.name)));

    auto* members = new QTableWidgetItem(MembersText(role.members));
    members->setForeground(role.members.empty() ? tokens.fg_subtle : tokens.fg);
    grid_->setItem(i, 1, members);

    // Well-known versus custom is the one structural fact about a Role an
    // administrator needs: a well-known Role's node cannot be deleted.
    // Role-specific source strings rather than a bare "Standard"/"Custom":
    // the existing "Custom" translation is masculine and would not agree with
    // the feminine "роль".
    auto* kind = new QTableWidgetItem(
        role.well_known ? Tr("Standard role") : Tr("Custom role"));
    kind->setForeground(tokens.fg_muted);
    grid_->setItem(i, 2, kind);
  }
}

RolesGridPanel* MakeRolesGridPanel() {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return nullptr;
  return new RolesGridPanel;
}
