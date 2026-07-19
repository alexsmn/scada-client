#pragma once

#include "scada/node_id.h"
#include "user_access/users_grid.h"

#include <QWidget>

#include <vector>

class QLabel;
class QPushButton;
class QTableWidget;

// The reshell users-admin grid — the main region of users-admin.html. It lists
// the users (identity + coarse role + session policy) in a themed table, with a
// header carrying the count and the "editing requires Administrator" hint, and
// disabled Add-user / Reset-password affordances (the create/reset write path is
// not wired here — see the module note). Selecting a row emits UserActivated so
// the host can drive the RBAC inspector.
//
// It is returned as the Users view under the reshell theme (see
// NodeTableController); UiView is a QWidget, so the panel is the view.
class UsersGridPanel : public QWidget {
  Q_OBJECT

 public:
  explicit UsersGridPanel(QWidget* parent = nullptr);
  ~UsersGridPanel() override;

  // Renders the given rows into the grid and updates the header count. The seam
  // the widget tests and the controller's browse both drive.
  void ShowRows(const std::vector<UserGridRow>& rows);

  const std::vector<UserGridRow>& rows() const { return rows_; }

 Q_SIGNALS:
  // Emitted when the operator activates (selects) a user row.
  void UserActivated(const scada::NodeId& user_id);

 private:
  QWidget* BuildHeader();
  QWidget* BuildGrid();
  void OnSelectionChanged();

  QLabel* title_ = nullptr;
  QPushButton* add_user_ = nullptr;
  QPushButton* reset_password_ = nullptr;
  QTableWidget* grid_ = nullptr;

  std::vector<UserGridRow> rows_;
};

// Builds a UsersGridPanel under the reshell UX theme
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy); returns nullptr in
// the legacy look. Ownership transfers to the caller.
UsersGridPanel* MakeUsersGridPanel();
