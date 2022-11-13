#pragma once

#include "controls/models/grid_model.h"

#include <Wt/WAbstractTableModel.h>
#include <memory>

class GridModelAdapter final : public Wt::WAbstractTableModel,
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
  const std::shared_ptr<aui::GridModel> model_;
  const std::shared_ptr<aui::HeaderModel> row_model_;
  const std::shared_ptr<aui::HeaderModel> column_model_;
};
