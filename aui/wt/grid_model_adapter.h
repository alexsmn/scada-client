#pragma once

#include "aui/models/grid_model.h"

#include <Wt/WAbstractTableModel.h>
#include <boost/signals2/connection.hpp>
#include <memory>
#include <vector>

namespace aui {

class GridModelAdapter final : public Wt::WAbstractTableModel {
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

  void OnGridModelChanged(GridModel& model);
  void OnGridRangeChanged(GridModel& model, const GridRange& range);
  void OnGridRowsAdded(GridModel& model, int first, int count);
  void OnGridRowsRemoved(GridModel& model, int first, int count);

  void OnModelChanged(HeaderModel& model);

 private:
  void ConnectModels();

  const std::shared_ptr<GridModel> model_;
  const std::shared_ptr<HeaderModel> row_model_;
  const std::shared_ptr<HeaderModel> column_model_;

  std::vector<boost::signals2::scoped_connection> model_connections_;
};

}  // namespace aui
