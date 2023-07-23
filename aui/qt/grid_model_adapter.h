#pragma once

#include "aui/models/grid_model.h"

#include <QtCore/qabstractitemmodel.h>
#include <memory>

namespace aui {

class GridModelAdapter final : public QAbstractTableModel,
                               private GridModel::Observer,
                               private ColumnHeaderModel::Observer {
 public:
  GridModelAdapter(std::shared_ptr<GridModel> model,
                   std::shared_ptr<HeaderModel> row_model,
                   std::shared_ptr<HeaderModel> column_model);
  ~GridModelAdapter();

  HeaderModel& row_model() { return *row_model_; }
  HeaderModel& column_model() { return *column_model_; }

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

  // GridModel::Observer
  virtual void OnGridModelChanged(GridModel& model) override;
  virtual void OnGridRangeChanged(GridModel& model,
                                  const GridRange& range) override;
  virtual void OnGridRowsAdded(GridModel& model, int first, int count) override;
  virtual void OnGridRowsRemoved(GridModel& model,
                                 int first,
                                 int count) override;

  // ColumnHeaderModel::Observer
  virtual void OnModelChanged(HeaderModel& model) override;

 private:
  std::u16string GetCsvData(const QModelIndexList& indexes) const;

  const std::shared_ptr<GridModel> model_;
  const std::shared_ptr<HeaderModel> row_model_;
  const std::shared_ptr<HeaderModel> column_model_;
};

}  // namespace aui
