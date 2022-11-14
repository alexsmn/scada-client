#pragma once

#include "controls/color.h"
#include "controls/handlers.h"
#include "controls/models/tree_model.h"
#include "ui/base/dragdrop/os_exchange_data.h"

#include <QAbstractitemmodel>
#include <memory>

class QIcon;

namespace aui {

class TreeModel;

class TreeModelAdapter : public QAbstractItemModel, private TreeModelObserver {
 public:
  explicit TreeModelAdapter(std::shared_ptr<TreeModel> model);
  virtual ~TreeModelAdapter();

  void SetCheckable(bool checkable) { checkable_ = checkable; }

  using CheckedHandler = std::function<void(void* node, bool checked)>;
  void SetCheckedHandler(CheckedHandler handler) {
    checked_handler_ = std::move(handler);
  }

  bool IsChecked(void* node) const;
  void SetChecked(void* node, bool checked);
  void SetCheckedNodes(std::set<void*> nodes);

  void LoadIcons(unsigned resource_id, int width, Color mask_color);

  void* GetNode(const QModelIndex& index) const;
  QModelIndex GetNodeIndex(void* node, int column) const;

  void SetDragHandler(std::vector<std::string> supported_formats,
                      DragHandler handler);

  // QAbstractItemModel
  virtual QVariant headerData(int section,
                              Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(
      int row,
      int column,
      const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& child) const override;
  virtual int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index,
                        int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex& index,
                       const QVariant& value,
                       int role = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual bool hasChildren(const QModelIndex& parent) const override;
  virtual bool canFetchMore(const QModelIndex& parent) const override;
  virtual void fetchMore(const QModelIndex& parent) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;
  virtual bool canDropMimeData(const QMimeData* data,
                               Qt::DropAction action,
                               int row,
                               int column,
                               const QModelIndex& parent) const override;
  virtual bool dropMimeData(const QMimeData* data,
                            Qt::DropAction action,
                            int row,
                            int column,
                            const QModelIndex& parent) override;

  int row_height = 18;

  DropHandler drop_handler;

 private:
  int GetIndexOf(void* node) const;

  DropAction GetDropAction(const QMimeData* data,
                           Qt::DropAction action,
                           int row,
                           int column,
                           const QModelIndex& parent) const;

  // private TreeModelObserver
  virtual void OnTreeNodesAdding(void* parent, int start, int count) override;
  virtual void OnTreeNodesAdded(void* parent, int start, int count) override;
  virtual void OnTreeNodesDeleting(void* parent, int start, int count) override;
  virtual void OnTreeNodesDeleted(void* parent, int start, int count) override;
  virtual void OnTreeNodeChanged(void* node) override;
  virtual void OnTreeModelResetting() override;
  virtual void OnTreeModelReset() override;

  const std::shared_ptr<TreeModel> model_;

  std::vector<QIcon> icons_;

  bool checkable_ = false;
  CheckedHandler checked_handler_;
  std::set<void*> checked_nodes_;

  QStringList supported_mime_types_;
  DragHandler drag_handler_;
};

}  // namespace aui
