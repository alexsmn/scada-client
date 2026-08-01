#pragma once

#include "aui/color.h"
#include "aui/handlers.h"
#include "base/lifetime.h"
#include <boost/json.hpp>

#include <QPoint>
#include <QTableView>
#include <span>
#include <string_view>

class QSortFilterProxyModel;
class QEvent;

namespace scada::aui {

class TableModel;
class TableModelAdapter;
struct TableColumn;

class Table : public QTableView {
 public:
  Table(std::shared_ptr<TableModel> model,
        std::vector<TableColumn> columns,
        bool sorting = false);
  ~Table();

  const std::vector<TableColumn>& columns() const SCADA_LIFETIME_BOUND;

  void SetShowGrid(bool show_grid) { setShowGrid(show_grid); }

  void SetSelectionChangeHandler(SelectionChangeHandler handler);

  void SetContextMenuHandler(ContextMenuHandler handler);

  void SetDoubleClickHandler(DoubleClickHandler handler);

  void SetKeyPressHandler(KeyPressHandler handler);

  void SetStateChangeHandler(StateChangeHandler handler);

  int GetCurrentRow() const { return currentIndex().row(); }

  std::vector<int> GetSelectedRows() const;

  void SelectRow(int row, bool make_visible = true);

  bool editing() const { return state() == State::EditingState; };

  void OpenEditor(int row);
  void CloseEditor() {}

  QWidget* CreateParentIfNecessary() { return this; }

  // Loads cell glyphs from SVG resources, tinted to follow the palette and
  // re-tinted when the theme changes (docs/client/ux/iconography.md §5.2).
  void LoadGlyphs(std::span<const std::string_view> resource_paths, int size);

  boost::json::value SaveState() const;
  void RestoreState(const boost::json::value& data);

  // Shows or hides one column by its `TableColumn::id`. Hiding every column is
  // refused — the header context menu is the only way back, and an empty
  // header offers nothing to right-click.
  void SetColumnVisible(int column_id, bool visible);
  bool IsColumnVisible(int column_id) const;

  void CopyToClipbard();

 protected:
  // QTableView
  virtual void changeEvent(QEvent* event) override;
  virtual void keyPressEvent(QKeyEvent* event) override;

 private:
  void ApplyThemePalette();

  // The header's right-click menu: one checkable entry per column, the
  // conventional desktop idiom for choosing them.
  void ShowColumnMenu(const QPoint& position);

  // Visual position of `column_id`, or -1.
  int ColumnSection(int column_id) const;

  int VisibleColumnCount() const;

  QModelIndex RowToIndex(int row) const;
  int IndexToRow(const QModelIndex& index) const;

  std::unique_ptr<TableModelAdapter> model_adapter_;

  std::unique_ptr<QSortFilterProxyModel> proxy_model_;

  KeyPressHandler key_press_handler_;
};

}  // namespace scada::aui
