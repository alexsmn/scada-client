#pragma once

#include <QtCore/qabstractitemmodel.h>
#include <memory>

#include "ui/base/models/grid_model.h"

class GridModelAdapter final : public QAbstractTableModel,
                               private ui::GridModel::Observer,
                               private ui::ColumnHeaderModel::Observer {
 public:
  GridModelAdapter(std::shared_ptr<ui::GridModel> model,
                   std::shared_ptr<ui::HeaderModel> row_model,
                   std::shared_ptr<ui::HeaderModel> column_model);
  ~GridModelAdapter();

  ui::HeaderModel& row_model() { return *row_model_; }
  ui::HeaderModel& column_model() { return *column_model_; }

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
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

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
  std::u16string GetCsvData(const QModelIndexList& indexes) const;

  const std::shared_ptr<ui::GridModel> model_;
  const std::shared_ptr<ui::HeaderModel> row_model_;
  const std::shared_ptr<ui::HeaderModel> column_model_;
};
