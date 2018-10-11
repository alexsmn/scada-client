#pragma once

#include <QtCore/qabstractitemmodel.h>
#include <memory>

#include "ui/base/models/grid_model.h"

class GridModelAdapter final : public QAbstractTableModel,
                               private ui::GridModel::Observer,
                               private ui::ColumnHeaderModel::Observer {
 public:
  GridModelAdapter(ui::GridModel& model,
                   ui::HeaderModel& row_model,
                   ui::HeaderModel& column_model);
  virtual ~GridModelAdapter();

  // QAbstractTableModel
  virtual int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index,
                        int role = Qt::DisplayRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section,
                              Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index,
                       const QVariant& value,
                       int role) override;

  // ui::GridModel::Observer
  virtual void OnGridModelChanged(ui::GridModel& model) override;
  virtual void OnGridRangeChanged(ui::GridModel& model,
                                  const ui::GridRange& range) override;
  virtual void OnGridRowsAdded(ui::GridModel& model,
                               int first,
                               int count) override;
  virtual void OnGridRowsRemoved(ui::GridModel& model,
                                 int first,
                                 int count) override;

  // ui::ColumnHeaderModel::Observer
  virtual void OnModelChanged(ui::HeaderModel& model) override;

 private:
  ui::GridModel& model_;
  ui::HeaderModel& row_model_;
  ui::HeaderModel& column_model_;
};
