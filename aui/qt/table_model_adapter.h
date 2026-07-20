#pragma once

#include "aui/color.h"
#include "base/lifetime.h"

#include <QAbstractItemModel>
#include <boost/signals2/connection.hpp>
#include <memory>
#include <vector>

class QIcon;

namespace scada::aui {

class TableModel;
struct TableColumn;

class TableModelAdapter : public QAbstractTableModel {
 public:
  TableModelAdapter(std::shared_ptr<TableModel> model,
                    std::vector<TableColumn> columns);
  virtual ~TableModelAdapter();

  TableModel& model() SCADA_LIFETIME_BOUND { return *model_; }
  const TableModel& model() const SCADA_LIFETIME_BOUND { return *model_; }

  std::vector<TableColumn>& columns() SCADA_LIFETIME_BOUND { return columns_; }
  const std::vector<TableColumn>& columns() const SCADA_LIFETIME_BOUND {
    return columns_;
  }

  void LoadIcons(unsigned resource_id, int width, Color mask_color);

  // QAbstractTableModel
  virtual int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index,
                        int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index,
                       const QVariant& value,
                       int role = Qt::EditRole) override;
  virtual QVariant headerData(int section,
                              Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual void sort(int column,
                    Qt::SortOrder order = Qt::AscendingOrder) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

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
  std::vector<QIcon> icons_;

  std::vector<boost::signals2::scoped_connection> model_connections_;
};

}  // namespace aui
