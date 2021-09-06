#pragma once

#include "ui/base/models/tree_node_model.h"

#include <memory>

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <Wt/WAbstractItemModel.h>
#include <Wt/WIcon.h>
#pragma warning(pop)

namespace ui {
class TreeModel;
}

class TreeModelAdapter : public Wt::WAbstractItemModel,
                         private ui::TreeModelObserver {
 public:
  explicit TreeModelAdapter(std::shared_ptr<ui::TreeModel> model);
  virtual ~TreeModelAdapter();

  void SetCheckable(bool checkable) { checkable_ = checkable; }

  using CheckedHandler = std::function<void(void* node, bool checked)>;
  void SetCheckedHandler(CheckedHandler handler) {
    checked_handler_ = std::move(handler);
  }

  bool IsChecked(void* node) const;
  void SetChecked(void* node, bool checked);
  void SetCheckedNodes(std::set<void*> nodes);

  void LoadIcons(unsigned resource_id, int width, Wt::WColor mask_color);

  void* GetNode(const Wt::WModelIndex& index) const;
  Wt::WModelIndex GetNodeIndex(void* node, int column) const;

  // QAbstractItemModel
  virtual Wt::cpp17::any headerData(
      int section,
      Wt::Orientation orientation,
      Wt::ItemDataRole role = Wt::ItemDataRole::Display) const override;
  virtual Wt::WModelIndex index(
      int row,
      int column,
      const Wt::WModelIndex& parent = Wt::WModelIndex()) const override;
  virtual Wt::WModelIndex parent(const Wt::WModelIndex& child) const override;
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
  virtual Wt::WFlags<Wt::ItemFlag> flags(
      const Wt::WModelIndex& index) const override;

  int row_height = 18;

 private:
  int GetIndexOf(void* node) const;

  // private ui::TreeModelObserver
  virtual void OnTreeNodesAdding(void* parent, int start, int count) override;
  virtual void OnTreeNodesAdded(void* parent, int start, int count) override;
  virtual void OnTreeNodesDeleting(void* parent, int start, int count) override;
  virtual void OnTreeNodesDeleted(void* parent, int start, int count) override;
  virtual void OnTreeNodeChanged(void* node) override;
  virtual void OnTreeModelResetting() override;
  virtual void OnTreeModelReset() override;

  const std::shared_ptr<ui::TreeModel> model_;

  std::vector<Wt::WIcon> icons_;

  bool checkable_ = false;
  CheckedHandler checked_handler_;
  std::set<void*> checked_nodes_;
};
