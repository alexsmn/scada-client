#pragma once

#include "aui/models/grid_model.h"

#include <Wt/WAbstractTableModel.h>
#include <memory>

namespace aui {

class GridModelAdapter final : public Wt::WAbstractTableModel,
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
      const Wt::WModelIndex& parent = Wt::WModelIndex()) const override;
  virtual int columnCount(
      const Wt::WModelIndex& parent = Wt::WModelIndex()) const override;
  virtual Wt::cpp17::any data(
      const Wt::WModelIndex& index,
      Wt::ItemDataRole role = Wt::ItemDataRole::Display) const override;
  virtual Wt::WFlags<Wt::ItemFlag> flags(
      const Wt::WModelIndex& index) const override;
  virtual Wt::cpp17::any headerData(
      int section,
      Wt::Orientation orientation,
      Wt::ItemDataRole role = Wt::ItemDataRole::Display) const override;
  virtual bool setData(const Wt::WModelIndex& index,
                       const Wt::cpp17::any& value,
                       Wt::ItemDataRole role) override;

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
  const std::shared_ptr<GridModel> model_;
  const std::shared_ptr<HeaderModel> row_model_;
  const std::shared_ptr<HeaderModel> column_model_;
};

}  // namespace aui
