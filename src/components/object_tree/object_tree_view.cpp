#include "components/object_tree/object_tree_view.h"

#include "controller_factory.h"
#include "contents_model.h"
#include "components/object_tree/object_tree_model.h"
#include "components/main/main_window.h"
#include "components/main/opened_view.h"
#include "services/profile.h"
#include "controls/tree.h"
#include "common/scada_node_ids.h"
#include "core/session_service.h"

#if defined(UI_VIEWS)
#include "skia/ext/skia_utils_win.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "base/color_string.h"
#endif

REGISTER_CONTROLLER(ObjectTreeView, ID_OBJECT_VIEW);

ObjectTreeView::ObjectTreeView(const ControllerContext& context)
    : ConfigurationTreeView(context, std::make_unique<ObjectTreeModel>(
          context.node_service_,
          id::DataItems,
          context.timed_data_service_,
          context.profile_)) {
#if defined(UI_QT)
  tree_view().model_adapter().set_checkable(true);
  tree_view().setHeaderHidden(false);
#elif defined(UI_VIEWS)
  tree_view().set_custom_painter(this);
#endif

  tree_view().SetExpandedHandler([this](void* node, bool expanded) {
    UpdateNodesVisibility(*static_cast<ConfigurationTreeNode*>(node), expanded);
  });

  tree_view().SetCheckedHandler([this](void* node, bool checked) {
    ConfigurationTreeNode* n = reinterpret_cast<ConfigurationTreeNode*>(node);
    if (!n->data_node())
      return;
    auto* contents_model = controller_delegate_.GetActiveContentsModel();
    if (contents_model) {
      if (checked)
        contents_model->RemoveContainedItem(n->data_node().id());
      else
        contents_model->AddContainedItem(n->data_node().id(), ContentsModel::APPEND);
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

void ObjectTreeView::UpdateNodesVisibility(ConfigurationTreeNode& parent_node, bool expanded) {
  for (int i = 0; i < parent_node.GetChildCount(); ++i) {
    ConfigurationTreeNode& child = parent_node.GetChild(i);
    model().SetNodeVisible(child, expanded);
    if (tree_view().IsExpanded(&child, false))
      UpdateNodesVisibility(child, expanded);
  }
}

#if defined(UI_VIEWS)
void ObjectTreeView::OnPaintNode(gfx::Canvas* canvas,
                                 const gfx::Rect& node_bounds, void* node) {
  ConfigurationTreeNode& cfg_node = *model().AsNode(node);
  auto* timed_data = model().GetTimedData(&cfg_node);
  if (!timed_data)
    return;

  // TODO: Refactor.

  base::string16 value = timed_data->GetCurrentString(FORMAT_DEFAULT | FORMAT_COLOR);

  SkColor color = SK_ColorBLACK;
  if (timed_data->current().qualifier.general_bad())
    color = profile_.bad_value_color();

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

void ObjectTreeView::OnTreeNodesDeleted(void* parent, int start, int count) {
  ConfigurationTreeNode& parent_node = *model().AsNode(parent);
  if (tree_view().IsExpanded(&parent_node, true)) {
    for (int i = 0; i < count; ++i) {
      ConfigurationTreeNode& child = parent_node.GetChild(start + i);
      model().SetNodeVisible(child, false);
    }
  }
}

void ObjectTreeView::OnContainedItemsUpdate(const std::set<scada::NodeId>& item_ids) {
#if defined(UI_VIEWS)
  for (auto& p : model().node_map()) {
    ConfigurationTreeNode& node = *p.second;
    bool check = node.data_node() && item_ids.find(node.data_node().id()) != item_ids.end();
    tree_view().SetChecked(&node, check);
  }
#endif
}

void ObjectTreeView::OnContainedItemChanged(const scada::NodeId& item_id, bool added) {
#if defined(UI_VIEWS)
  ConfigurationTreeNode* node = model().FindNode(item_id);
  if (node)
    tree_view().SetChecked(node, added);
#endif
}
