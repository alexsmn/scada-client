#pragma once

#include <memory>
#include <qabstractitemmodel.h>
#include <qicon.h>

#include "ui/base/models/tree_node_model.h"

namespace ui {
class TreeModel;
}

class TreeModelAdapter : public QAbstractItemModel,
                         private ui::TreeModelObserver {
 public:
  explicit TreeModelAdapter(ui::TreeModel& model);
  virtual ~TreeModelAdapter();

  void set_checkable(bool checkable) { checkable_ = checkable; }

  void LoadIcons(unsigned resource_id, int width, QColor mask_color);

  void* GetNode(const QModelIndex& index) const;
  QModelIndex GetNodeIndex(void* node, int column) const;

  // QAbstractItemModel
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex &child) const override;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

 private:
  int GetIndexOf(void* node) const;

  // private ui::TreeModelObserver
  virtual void OnTreeNodesAdded(void* parent, int start, int count) override;
  virtual void OnTreeNodesDeleted(void* parent, int start, int count) override;
  virtual void OnTreeNodeChanged(void* node) override;

  ui::TreeModel& model_;

  std::vector<QIcon> icons_;

  bool checkable_;
  std::set<void*> checked_nodes_;
};