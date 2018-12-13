#include "components/object_tree/object_tree_view.h"

#include "client_utils.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "components/object_tree/object_tree_model.h"
#include "contents_model.h"
#include "controller_factory.h"
#include "controls/tree.h"
#include "services/profile.h"

#if defined(UI_VIEWS)
#include "base/color_string.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#endif

const WindowInfo kWindowInfo = {
    ID_OBJECT_VIEW, "Struct", L"Объекты", WIN_SING, 200, 400, 0};

REGISTER_CONTROLLER(ObjectTreeView, kWindowInfo);

ObjectTreeView::ObjectTreeView(const ControllerContext& context)
    : ConfigurationTreeView{
          context, *new ObjectTreeModel{ObjectTreeModelContext{
                       context.node_service_, context.task_manager_,
                       context.node_service_.GetNode(id::DataItems),
                       context.timed_data_service_, context.profile_}}} {
#if defined(UI_QT)
  tree_view().setHeaderHidden(false);
#elif defined(UI_VIEWS)
  tree_view().set_custom_painter(this);
#endif

  tree_view().SetShowChecks(true);

  tree_view().SetExpandedHandler([this](void* node, bool expanded) {
    UpdateNodesVisibility(*static_cast<ConfigurationTreeNode*>(node), expanded);
  });

  tree_view().SetCheckedHandler([this](void* node, bool checked) {
    ConfigurationTreeNode* n = reinterpret_cast<ConfigurationTreeNode*>(node);
    auto* contents_model = controller_delegate_.GetActiveContentsModel();
    if (contents_model) {
      NodeIdSet node_ids;
      ExpandGroupItemIds(n->data_node(), node_ids);
      for (auto& node_id : node_ids) {
        if (checked)
          contents_model->AddContainedItem(node_id, ContentsModel::APPEND);
        else
          contents_model->RemoveContainedItem(node_id);
      }
    }
  });

  controller_delegate_.AddContentsObserver(*this);

  model().AddObserver(*this);
}

ObjectTreeView::~ObjectTreeView() {
  controller_delegate_.RemoveContentsObserver(*this);

  model().RemoveObserver(*this);

#if defined(UI_VIEWS)
  tree_view().set_custom_painter(NULL);
#endif
}

ObjectTreeModel& ObjectTreeView::model() {
  return static_cast<ObjectTreeModel&>(ConfigurationTreeView::model());
}

void ObjectTreeView::UpdateNodesVisibility(ConfigurationTreeNode& parent_node,
                                           bool expanded) {
  for (int i = 0; i < parent_node.GetChildCount(); ++i) {
    ConfigurationTreeNode& child = parent_node.GetChild(i);
    model().SetNodeVisible(child, expanded);
    if (tree_view().IsExpanded(&child, false))
      UpdateNodesVisibility(child, expanded);
  }
}

#if defined(UI_VIEWS)
void ObjectTreeView::OnPaintNode(gfx::Canvas* canvas,
                                 const gfx::Rect& node_bounds,
                                 void* node) {
  ConfigurationTreeNode& cfg_node = *model().AsNode(node);
  auto* timed_data = model().GetTimedData(&cfg_node);
  if (!timed_data)
    return;

  // TODO: Refactor.

  base::string16 value =
      timed_data->GetCurrentString(FORMAT_DEFAULT | FORMAT_COLOR);

  SkColor color = SK_ColorBLACK;
  if (timed_data->current().qualifier.general_bad())
    color = profile_.bad_value_color;

  RECT rc = node_bounds.ToRECT();

  RECT text_rect = rc;
  const gfx::Font& font = ui::ResourceBundle::GetSharedInstance().GetFont(
      ui::ResourceBundle::BASE_FONT);
  MeasureColoredString(canvas, font, color, text_rect, value,
                       DT_RIGHT | DT_SINGLELINE | DT_VCENTER);

  gfx::Rect fill_rect = node_bounds;
  fill_rect.set_x(text_rect.left);
  fill_rect.set_width(text_rect.right - text_rect.left);
  SkColor fill_color = skia::COLORREFToSkColor(GetSysColor(COLOR_WINDOW));
  if (Blinker::GetState() && timed_data->alerting())
    fill_color = SK_ColorYELLOW;
  canvas->FillRect(fill_rect, fill_color);

  rc.right -= 5;
  // draw text
  DrawColoredString(canvas, font, color, rc, value,
                    DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
}
#endif

void ObjectTreeView::OnTreeNodesAdded(void* parent, int start, int count) {
  ConfigurationTreeNode& parent_node = *model().AsNode(parent);
  if (tree_view().IsExpanded(&parent_node, true)) {
    for (int i = 0; i < count; ++i) {
      ConfigurationTreeNode& child = parent_node.GetChild(start + i);
      model().SetNodeVisible(child, true);
    }
  }
}

void ObjectTreeView::OnTreeNodesDeleting(void* parent, int start, int count) {
  ConfigurationTreeNode& parent_node = *model().AsNode(parent);
  if (tree_view().IsExpanded(&parent_node, true)) {
    for (int i = 0; i < count; ++i) {
      ConfigurationTreeNode& child = parent_node.GetChild(start + i);
      model().SetNodeVisible(child, false);
    }
  }

  for (int i = 0; i < count; ++i) {
    ConfigurationTreeNode& child = parent_node.GetChild(start + i);
    tree_view().SetChecked(&child, false);
  }
}

void ObjectTreeView::OnContentsChanged(const NodeIdSet& node_ids) {
  std::set<void*> checked_nodes;

  std::vector<void*> pending_nodes;
  pending_nodes.reserve(node_ids.size());
  for (auto& node_id : node_ids) {
    if (auto* node = model().FindNode(node_id))
      pending_nodes.emplace_back(node);
  }

  std::unordered_map<void* /*parent*/, int /*checked_child_count*/>
      parent_checks;

  while (!pending_nodes.empty()) {
    // Allow adding nulls for performance and remove it then.
    for (auto* node : pending_nodes)
      ++parent_checks[model().GetParent(node)];
    parent_checks.erase(nullptr);

    checked_nodes.insert(pending_nodes.begin(), pending_nodes.end());
    pending_nodes.clear();

    for (auto [parent, checked_child_count] : parent_checks) {
      int child_count = model().GetChildCount(parent);
      if (child_count == checked_child_count)
        pending_nodes.emplace_back(parent);
    }
    parent_checks.clear();
  }

  tree_view().SetCheckedNodes(std::move(checked_nodes));
}

void ObjectTreeView::OnContainedItemChanged(const scada::NodeId& item_id,
                                            bool added) {
  void* node = model().FindNode(item_id);
  while (node && tree_view().IsChecked(node) != added) {
    tree_view().SetChecked(node, added);

    auto* parent = model().GetParent(node);
    if (!parent)
      break;

    if (added) {
      bool all_children_checked = true;
      for (int i = 0; i < model().GetChildCount(parent); ++i) {
        auto* child = model().GetChild(parent, i);
        if (!tree_view().IsChecked(child)) {
          all_children_checked = false;
          break;
        }
      }

      if (!all_children_checked)
        break;
    }

    node = parent;
  }
}
