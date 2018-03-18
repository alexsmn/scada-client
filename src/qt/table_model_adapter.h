#pragma once

#include <QtCore/qabstractitemmodel.h>
#include <memory>

#include "ui/base/models/table_model_observer.h"

namespace ui {
class TableModel;
struct TableColumn;
}  // namespace ui

class TableModelAdapter : public QAbstractTableModel,
                          private ui::TableModelObserver {
 public:
  TableModelAdapter(ui::TableModel& model,
                    std::vector<ui::TableColumn> columns);
  virtual ~TableModelAdapter();

  // QAbstractTableModel
  virtual int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index,
                        int role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int section,
                              Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const override;

  // ui::TableModelObserver
  virtual void OnModelChanged() override;
  virtual void OnItemsChanged(int first, int count) override;
  virtual void OnItemsAdded(int first, int count) override;
  virtual void OnItemsRemoved(int first, int count) override;

 private:
  ui::TableModel& model_;
  std::vector<ui::TableColumn> columns_;
};
