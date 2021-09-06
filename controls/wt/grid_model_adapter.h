#pragma once

#include "ui/base/models/grid_model.h"

#include <memory>

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <Wt/WAbstractTableModel.h>
#pragma warning(pop)

class GridModelAdapter final : public Wt::WAbstractTableModel,
                               private ui::GridModel::Observer,
                               private ui::ColumnHeaderModel::Observer {
 public:
  GridModelAdapter(ui::GridModel& model,
                   ui::HeaderModel& row_model,
                   ui::HeaderModel& column_model);
  ~GridModelAdapter();

  ui::HeaderModel& row_model() { return row_model_; }
  ui::HeaderModel& column_model() { return column_model_; }

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
