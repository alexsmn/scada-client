#pragma once

#include "ui/base/models/table_model_observer.h"

#include <QAbstractItemModel>
#include <memory>

namespace ui {
class TableModel;
struct TableColumn;
}  // namespace ui

class QIcon;

class TableModelAdapter : public QAbstractTableModel,
                          private ui::TableModelObserver {
 public:
  TableModelAdapter(std::shared_ptr<ui::TableModel> model,
                    std::vector<ui::TableColumn> columns);
  virtual ~TableModelAdapter();

  ui::TableModel& model() { return *model_; }
  const ui::TableModel& model() const { return *model_; }

  std::vector<ui::TableColumn>& columns() { return columns_; }
  const std::vector<ui::TableColumn>& columns() const { return columns_; }

  void LoadIcons(unsigned resource_id, int width, QColor mask_color);

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

  // ui::TableModelObserver
  virtual void OnModelChanged() override;
  virtual void OnItemsChanged(int first, int count) override;
  virtual void OnItemsAdding(int first, int count) override;
  virtual void OnItemsAdded(int first, int count) override;
  virtual void OnItemsRemoving(int first, int count) override;
  virtual void OnItemsRemoved(int first, int count) override;

 private:
  const std::shared_ptr<ui::TableModel> model_;
  std::vector<ui::TableColumn> columns_;
  std::vector<QIcon> icons_;
};
