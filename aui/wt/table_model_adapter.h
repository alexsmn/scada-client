#pragma once

#include <Wt/WAbstractTableModel.h>
#include <boost/signals2/connection.hpp>
#include <memory>
#include <vector>

namespace scada::aui {

class TableModel;
struct TableColumn;

class TableModelAdapter : public Wt::WAbstractTableModel {
 public:
  TableModelAdapter(std::shared_ptr<TableModel> model,
                    std::vector<TableColumn> columns);
  virtual ~TableModelAdapter();

  TableModel& model() { return *model_; }
  const TableModel& model() const { return *model_; }

  std::vector<TableColumn>& columns() { return columns_; }
  const std::vector<TableColumn>& columns() const { return columns_; }

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

  void OnModelChanged();
  void OnItemsChanged(int first, int count);
  void OnItemsAdding(int first, int count);
  void OnItemsAdded(int first, int count);
  void OnItemsRemoving(int first, int count);
  void OnItemsRemoved(int first, int count);

 private:
  void ConnectModel();

  const std::shared_ptr<TableModel> model_;
  std::vector<TableColumn> columns_;

  std::vector<boost::signals2::scoped_connection> model_connections_;
};

}  // namespace aui
