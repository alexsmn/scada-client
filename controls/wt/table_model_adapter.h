#pragma once

#include "controls/models/table_model_observer.h"

#include <Wt/WAbstractTableModel.h>
#include <memory>

namespace aui {
class TableModel;
struct TableColumn;
}  // namespace aui

class TableModelAdapter : public Wt::WAbstractTableModel,
                          private aui::TableModelObserver {
 public:
  TableModelAdapter(std::shared_ptr<aui::TableModel> model,
                    std::vector<aui::TableColumn> columns);
  virtual ~TableModelAdapter();

  aui::TableModel& model() { return *model_; }
  const aui::TableModel& model() const { return *model_; }

  std::vector<aui::TableColumn>& columns() { return columns_; }
  const std::vector<aui::TableColumn>& columns() const { return columns_; }

  // QAbstractTableModel
  virtual int rowCount(
      const Wt::WModelIndex& parent = Wt::WModelIndex()) const override;
  virtual int columnCount(
      const Wt::WModelIndex& parent = Wt::WModelIndex()) const override;
  virtual Wt::cpp17::any data(
      const Wt::WModelIndex& index,
      Wt::ItemDataRole role = Wt::ItemDataRole::Display) const override;
  virtual bool setData(const Wt::WModelIndex& index,
                       const Wt::cpp17::any& value,
                       Wt::ItemDataRole role = Wt::ItemDataRole::Edit) override;
  virtual Wt::cpp17::any headerData(
      int section,
      Wt::Orientation orientation = Wt::Orientation::Horizontal,
      Wt::ItemDataRole role = Wt::ItemDataRole::Display) const override;
  virtual Wt::WFlags<Wt::ItemFlag> flags(
      const Wt::WModelIndex& index) const override;

  // aui::TableModelObserver
  virtual void OnModelChanged() override;
  virtual void OnItemsChanged(int first, int count) override;
  virtual void OnItemsAdding(int first, int count) override;
  virtual void OnItemsAdded(int first, int count) override;
  virtual void OnItemsRemoving(int first, int count) override;
  virtual void OnItemsRemoved(int first, int count) override;

 private:
  const std::shared_ptr<aui::TableModel> model_;
  std::vector<aui::TableColumn> columns_;
};
