#pragma once

#include <set>

#include "client/components/configuration_tree/configuration_tree_view.h"
#include "client/contents_observer.h"
#include "ui/views/controls/tree/tree_view.h"

class ConfigurationTreeNode;
class ObjectTreeModel;

class ObjectTreeView : public ConfigurationTreeView,
#if defined(UI_VIEWS)
                       private views::TreeView::CustomPainter,
#endif
                       protected ui::TreeModelObserver,
                       private ContentsObserver {
 public:
  explicit ObjectTreeView(const ControllerContext& context);
  virtual ~ObjectTreeView();

 protected:
  void UpdateNodesVisibility(ConfigurationTreeNode& parent_node, bool expanded);

  // TreeModelObserver
  virtual void OnTreeNodesAdded(void* parent, int start, int count) override;
  virtual void OnTreeNodesDeleted(void* parent, int start, int count) override;

 private:
  ObjectTreeModel& model();

#if defined(UI_VIEWS)
  // views::TreeView::CustomPainter
  virtual void OnPaintNode(gfx::Canvas* canvas, const gfx::Rect& node_bounds, void* node) override;
#endif

  // ContentsObserver
  virtual void OnContainedItemsUpdate(const std::set<scada::NodeId>& item_ids) override;
  virtual void OnContainedItemChanged(const scada::NodeId& item_id, bool added) override;
};
