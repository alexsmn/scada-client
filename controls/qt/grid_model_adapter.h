#pragma once

#include <QtCore/qabstractitemmodel.h>
#include <memory>

#include "controls/models/grid_model.h"

class GridModelAdapter final : public QAbstractTableModel,
                               private aui::GridModel::Observer,
                               private aui::ColumnHeaderModel::Observer {
 public:
  GridModelAdapter(std::shared_ptr<aui::GridModel> model,
                   std::shared_ptr<aui::HeaderModel> row_model,
                   std::shared_ptr<aui::HeaderModel> column_model);
  ~GridModelAdapter();

  aui::HeaderModel& row_model() { return *row_model_; }
  aui::HeaderModel& column_model() { return *column_model_; }

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

  // aui::GridModel::Observer
  virtual void OnGridModelChanged(aui::GridModel& model) override;
  virtual void OnGridRangeChanged(aui::GridModel& model,
                                  const aui::GridRange& range) override;
  virtual void OnGridRowsAdded(aui::GridModel& model,
                               int first,
                               int count) override;
  virtual void OnGridRowsRemoved(aui::GridModel& model,
                                 int first,
                                 int count) override;

  // aui::ColumnHeaderModel::Observer
  virtual void OnModelChanged(aui::HeaderModel& model) override;

 private:
  std::u16string GetCsvData(const QModelIndexList& indexes) const;

  const std::shared_ptr<aui::GridModel> model_;
  const std::shared_ptr<aui::HeaderModel> row_model_;
  const std::shared_ptr<aui::HeaderModel> column_model_;
};
