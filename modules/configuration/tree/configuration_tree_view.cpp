#include "configuration/tree/configuration_tree_view.h"

#include "aui/tree.h"
#include "aui/translation.h"
#include "resources/common_resources.h"
#include "resources/icon_strips.h"
#include "configuration/tree/configuration_tree_drop_handler.h"
#include "configuration/tree/configuration_tree_model.h"
#include "controller/controller_delegate.h"
#include "ui/dragdrop/item_drag_data.h"
#include "node_service/node_util.h"
#include "profile/window_definition.h"

#if defined(UI_QT)
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"

#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#endif

namespace {

int CompareNodes(const NodeRef& a, const NodeRef& b) {
  if (!!a != !!b)
    return !!a < !!b ? 1 : -1;
  if (a.fetched() != b.fetched())
    return a.fetched() < b.fetched() ? 1 : -1;
  const auto& ta = a.type_definition().node_id();
  const auto& tb = b.type_definition().node_id();
  bool fa = a.node_class() != scada::NodeClass::Variable;
  bool fb = b.node_class() != scada::NodeClass::Variable;
  if (fa != fb)
    return fa < fb ? 1 : -1;
  if (ta != tb)
    return ta < tb ? -1 : 1;
  return ToString16(a.display_name()).compare(ToString16(b.display_name()));
}

}  // namespace

#if defined(UI_QT)
namespace {

// The active reshell theme's tokens. The filter field is only built under a
// token theme (the legacy path returns the bare tree), so the default is
// harmless.
const scada::aui::ThemeTokens& ExplorerTokens() {
  return scada::aui::ActiveThemeTokens();
}

// Wraps `tree` in a container with a type-to-filter field above it — the
// Explorer "Filter" search box from the reshell mockups
// (docs/product/ui-mockups/screens/config-workbench.html). Ownership of `tree`
// transfers into the returned container via Qt parent-child, preserving the
// caller-owns-the-returned-view contract.
std::unique_ptr<UiView> WrapExplorerWithFilter(scada::aui::Tree* tree) {
  const scada::aui::ThemeTokens& tokens = ExplorerTokens();
  auto container = std::make_unique<QWidget>();
  auto* layout = new QVBoxLayout{container.get()};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto* filter = new QLineEdit;
  filter->setObjectName(QStringLiteral("explorerFilter"));
  filter->setClearButtonEnabled(true);
  filter->setPlaceholderText(QString::fromStdU16String(Translate("Filter")));
  filter->setStyleSheet(
      QStringLiteral("QLineEdit{background:%1;border:1px solid %2;"
                     "border-radius:4px;padding:4px 8px;margin:6px 8px;"
                     "color:%3;}")
          .arg(tokens.surface_muted.name(), tokens.border.name(),
               tokens.fg.name()));
  QObject::connect(filter, &QLineEdit::textChanged, tree,
                   [tree](const QString& text) {
                     tree->SetFilterText(text.toStdU16String());
                   });
  layout->addWidget(filter);
  layout->addWidget(tree);
  return container;
}

}  // namespace
#endif

ConfigurationTreeView::ConfigurationTreeView(
    const ControllerContext& context,
    std::shared_ptr<ConfigurationTreeModel> model,
    std::unique_ptr<ConfigurationTreeDropHandler> drop_handler)
    : ControllerContext{context},
      model_{std::move(model)},
      drop_handler_{std::move(drop_handler)} {
  // cppcheck-suppress noCopyConstructor
  // cppcheck-suppress noOperatorEq
  tree_view_ = new scada::aui::Tree{model_};
  tree_view_->LoadGlyphs(kItemGlyphs, kTreeGlyphSize);
  tree_view_->SetRootVisible(true);
  tree_view_->SetSorted(true);

  tree_view_->SetFocusHandler([this] { controller_delegate_.Focus(); });

  tree_view_->SetSelectionChangedHandler([this] { UpdateSelection(); });

  tree_view_->SetDoubleClickHandler([this] {
    const auto& node = selection_.node();
    if (node)
      controller_delegate_.ExecuteDefaultNodeCommand(node);
  });

  tree_view_->SetCompareHandler([](void* left, void* right) {
    return CompareNodes(
        static_cast<const ConfigurationTreeNode*>(left)->node(),
        static_cast<const ConfigurationTreeNode*>(right)->node());
  });

  tree_view_->SetDragHandler(
      {std::string{ItemDragData::kMimeType}},
      [this](const std::vector<void*>& nodes) { return GetDragData(nodes); });

  tree_view_->SetDropHandler([this](int drop_action, const DragData& drag_data,
                                    void* target_node) {
    DropAction action;
    auto* target_tree_node = static_cast<ConfigurationTreeNode*>(target_node);
    drop_handler_->GetDropAction(drag_data, target_tree_node, action);
    return action;
  });

  tree_view_->SetContextMenuHandler([this](const scada::aui::Point& point) {
    // No view-specific static items: the tree's node commands are supplied by
    // the generic cross-platform context menu (the former `IDR_ITEM_POPUP`
    // carried only the dynamic `<Item>` placeholder).
    controller_delegate_.ShowPopupMenu(nullptr, point, true);
  });
}

ConfigurationTreeView::~ConfigurationTreeView() {}

std::unique_ptr<UiView> ConfigurationTreeView::Init(
    const WindowDefinition& definition) {
  if (auto* state = definition.FindItem("State"))
    tree_view_->RestoreState(state->attributes);

#if defined(UI_QT)
  // Reshell: a type-to-filter field above the Explorer tree. Opt-in on the
  // active UX theme; the legacy look keeps the bare tree.
  if (scada::aui::GetSeverityTheme() != scada::aui::SeverityTheme::kLegacy)
    return WrapExplorerWithFilter(tree_view_);
#endif

  return std::unique_ptr<UiView>{tree_view_};
}

void ConfigurationTreeView::Save(WindowDefinition& definition) {
  definition.AddItem("State").attributes = tree_view_->SaveState();
}

void ConfigurationTreeView::OnViewNodeCreated(const NodeRef& node) {
  // Select a first tree node.
  auto* tree_node = model_->FindFirstTreeNode(node.node_id());
  if (tree_node)
    tree_view().SelectNode(tree_node);
}

// Must keep |nodes| order.
std::vector<scada::NodeId> ConfigurationTreeView::GetVariableNodeIds(
    const std::vector<void*>& nodes) const {
  std::vector<scada::NodeId> node_ids;
  for (auto* node : nodes) {
    auto& n = *static_cast<ConfigurationTreeNode*>(node);
    if (n.node().node_class() == scada::NodeClass::Variable)
      node_ids.emplace_back(n.node().node_id());
  }
  return node_ids;
}

std::optional<OpenContext> ConfigurationTreeView::GetOpenContext() const {
  auto* tree_node =
      static_cast<ConfigurationTreeNode*>(tree_view().GetSelectedNode());
  if (!tree_node)
    tree_node = static_cast<ConfigurationTreeNode*>(model().GetRoot());
  if (!tree_node)
    return std::nullopt;

  OpenContext context;
  context.node = tree_node->node();
  return context;
}

void ConfigurationTreeView::UpdateSelection() {
  auto selection_size = tree_view_->GetSelectionSize();
  if (selection_size == 0)
    selection_.SelectNode(model_->root_node());
  else if (selection_size == 1) {
    auto* node =
        static_cast<ConfigurationTreeNode*>(tree_view_->GetSelectedNode());
    selection_.SelectNode(node ? node->node() : nullptr);
  } else
    selection_.SelectMultiple();
}

DragData ConfigurationTreeView::GetDragData(
    const std::vector<void*>& nodes) const {
  if (nodes.empty())
    return {};

  auto* tree_node = static_cast<ConfigurationTreeNode*>(nodes.front());
  auto node_id = tree_node->node().node_id();
  if (node_id.is_null())
    return {};

  DragData drag_data;
  ItemDragData{std::move(node_id)}.Save(drag_data);
  return drag_data;
}
