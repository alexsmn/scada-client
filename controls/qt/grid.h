#pragma once

#include "controls/types.h"
#include "qt/grid_model_adapter.h"
#include "ui/base/models/grid_model.h"

#include <QItemDelegate>
#include <QTableView>

class Grid : public QTableView {
 public:
  Grid(ui::GridModel& model,
       ui::HeaderModel& row_model,
       ui::HeaderModel& column_model);
  ~Grid();

  QWidget* CreateParentIfNecessary() { return this; }

  void SetContextMenuHandler(ContextMenuHandler handler);

 private:
  GridModelAdapter model_adapter_;

  ContextMenuHandler context_menu_handler_;

  class ItemDelegate final : public QItemDelegate {
   public:
    explicit ItemDelegate(Grid& grid) : grid_{grid} {}

    // QItemDelegate
    virtual QWidget* createEditor(QWidget* parent,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const override;
    virtual void setEditorData(QWidget* editor,
                               const QModelIndex& index) const override;
    virtual void setModelData(QWidget* editor,
                              QAbstractItemModel* model,
                              const QModelIndex& index) const override;

   private:
    void CommitAndCloseEditor();

    Grid& grid_;
  };

  ui::GridModel& model_;

  ItemDelegate item_delegate_{*this};
};
