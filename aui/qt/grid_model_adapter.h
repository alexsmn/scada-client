#pragma once

#include "aui/models/grid_model.h"
#include "base/lifetime.h"

#include <QtCore/qabstractitemmodel.h>
#include <boost/signals2/connection.hpp>
#include <memory>
#include <vector>

namespace scada::aui {

class GridModelAdapter final : public QAbstractTableModel {
 public:
  GridModelAdapter(std::shared_ptr<GridModel> model,
                   std::shared_ptr<HeaderModel> row_model,
                   std::shared_ptr<HeaderModel> column_model);
  ~GridModelAdapter();

  HeaderModel& row_model() SCADA_LIFETIME_BOUND { return *row_model_; }
  HeaderModel& column_model() SCADA_LIFETIME_BOUND { return *column_model_; }

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

  void OnGridModelChanged(GridModel& model);
  void OnGridRangeChanged(GridModel& model, const GridRange& range);
  void OnGridRowsAdding(GridModel& model, int first, int count);
  void OnGridRowsAdded(GridModel& model, int first, int count);
  void OnGridRowsRemoving(GridModel& model, int first, int count);
  void OnGridRowsRemoved(GridModel& model, int first, int count);

  void OnModelChanged(HeaderModel& model);

 private:
  std::u16string GetCsvData(const QModelIndexList& indexes) const;

  void ConnectModels();

  // Emits a `beginResetModel`/`endResetModel` pair and re-reads the section
  // counts it caches.
  void ResetFromModel();

  const std::shared_ptr<GridModel> model_;
  const std::shared_ptr<HeaderModel> row_model_;
  const std::shared_ptr<HeaderModel> column_model_;

  // The section counts as the views last saw them. A `HeaderModel` announces a
  // replacement without saying whether the count moved, and the answer decides
  // between a structural reset and a repaint -- so the previous count has to
  // be remembered here.
  int last_row_count_ = 0;
  int last_column_count_ = 0;

  std::vector<boost::signals2::scoped_connection> model_connections_;
};

}  // namespace scada::aui
