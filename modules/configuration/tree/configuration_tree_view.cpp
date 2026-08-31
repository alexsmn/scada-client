#include "configuration/tree/configuration_tree_view.h"

#include "aui/translation.h"
#include "aui/tree.h"
#include "configuration/tree/configuration_tree_drop_handler.h"
#include "configuration/tree/configuration_tree_model.h"
#include "controller/controller_delegate.h"
#include "node_service/node_util.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "resources/icon_strips.h"
#include "ui/dragdrop/item_drag_data.h"

#if defined(UI_QT)
#include "aui/severity_colors.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>
#endif

namespace {

int CompareNodes(const NodeRef& a, const NodeRef& b) {
  if (!!a != !!b)
    return !!a < !!b ? 1 : -1;
  if (a.fetched() != b.fetched())
    return a.fetched() < b.fetched() ? 1 : -1;
  // Hold the type-definition cursors, don't bind references into them.
  // type_definition() returns a NodeRef by value and node_id() is
  // SCADA_LIFETIME_BOUND to it, so a `const auto&` here dangled from the end of
  // its own full-expression — and both are read further down, which made this
  // comparator sort the Explorer's rows on freed memory. Clang says so
  // (-Wdangling); the annotation is what lets it.
  const NodeRef type_a = a.type_definition();
  const NodeRef type_b = b.type_definition();
  const scada::NodeId& ta = type_a.node_id();
  const scada::NodeId& tb = type_b.node_id();
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

// The Explorer's type-to-filter field.
//
// It draws no frame while the tree is unfiltered. An empty filter changes
// nothing about what the operator is looking at, so it should read as an
// affordance and no more -- the placeholder alone, in
// QPalette::PlaceholderText. The frame appears exactly when the field is being
// used or is changing what the tree shows: on hover, on focus, or while it
// holds text. That keeps it discoverable without giving a control that is doing
// nothing the same weight as the data below it (docs/client/ux/principles.md --
// chrome must not compete with process data).
//
// Everything here is the platform's. The frame is QLineEdit's own, the
// placeholder colour is the palette's, and the inset comes from the style's
// layout metrics. The previous version set a stylesheet with baked token
// colours, a hand-tuned radius and px padding, transcribed from the mockup's
// CSS -- which is the one thing the screens are not for: they are the
// information-architecture reference, and appearance is the host platform's
// (docs/client/ux/README.md, and the native direction agreed 2026-07-26).
class ExplorerFilterField : public QLineEdit {
 public:
  explicit ExplorerFilterField(QWidget* parent) : QLineEdit{parent} {
    setClearButtonEnabled(true);
    setPlaceholderText(QString::fromStdU16String(Translate("Filter")));
    setAttribute(Qt::WA_Hover, true);

    // Pin the height to the framed size before dropping the frame, so
    // revealing it later cannot make the field -- and the tree under it --
    // jump by the frame width.
    setFixedHeight(sizeHint().height());
    UpdateQuietState();

    connect(this, &QLineEdit::textChanged, this,
            [this] { UpdateQuietState(); });
  }

 protected:
  void enterEvent(QEnterEvent* event) override {
    QLineEdit::enterEvent(event);
    UpdateQuietState();
  }
  void leaveEvent(QEvent* event) override {
    QLineEdit::leaveEvent(event);
    UpdateQuietState();
  }
  void focusInEvent(QFocusEvent* event) override {
    QLineEdit::focusInEvent(event);
    UpdateQuietState();
  }
  void focusOutEvent(QFocusEvent* event) override {
    QLineEdit::focusOutEvent(event);
    UpdateQuietState();
  }

 private:
  // Frame only. The fill is deliberately left alone: it is drawn neither from
  // this widget's QPalette::Base nor from anything else reachable here --
  // verified by setting Base to pure red under the token theme and seeing the
  // field render unchanged. So the idle field keeps the theme's input surface,
  // which is DARKER than the pane and therefore recedes; what made the old
  // field shout was a *lighter* fill plus a bright outline, and both are gone.
  // Chasing an exact match would mean a stylesheet, which is the thing this
  // replaced.
  void UpdateQuietState() {
    const bool active = hasFocus() || underMouse() || !text().isEmpty();
    if (active == active_)
      return;
    active_ = active;
    setFrame(active);
  }

  bool active_ = true;
};

// Wraps `tree` in a container with the filter field above it -- the Explorer
// "Filter" box the screens draw over every sidebar tree
// (docs/product/ui-mockups/screens/config-workbench.html). Ownership of `tree`
// transfers into the returned container via Qt parent-child, preserving the
// caller-owns-the-returned-view contract.
std::unique_ptr<UiView> WrapExplorerWithFilter(scada::aui::Tree* tree) {
  auto container = std::make_unique<QWidget>();
  auto* layout = new QVBoxLayout{container.get()};
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // The field is inset, the tree is not: the tree is the pane's content and
  // runs to the pane edge, which is what every sidebar on the screens shows.
  // So the inset goes on a row of its own rather than on the shared layout,
  // and it comes from the style's layout metrics rather than the mockup's px.
  // Inset from the FONT, not from PM_LayoutLeftMargin. That metric is the
  // margin for a dialog's outer edge -- 12px a side under the macOS style,
  // measured -- which around a 21px field is more surround than control, and
  // it was what made the idle field read as a heavy block. Half a line height
  // is the unit here: it tracks the OS font-size setting, which is the reason
  // the native rule asks for a metric rather than a px constant, and it is
  // roughly the proportion the screens draw (8-10px beside a 24px row). The
  // bottom is tighter than the top because the tree follows immediately.
  // Half a line beside the field, a quarter above and below it. The vertical
  // budget is what shows: the field is only ~21px tall, so a 12px surround --
  // PM_LayoutLeftMargin, the dialog-edge metric, measured under the macOS
  // style -- is more chrome than control and pushes the tree down by more than
  // the field occupies. Measured against the rendered pane: the tree's header
  // starts at y=30 here, against y=32 for the stylesheet this replaced and
  // y=42 for a first attempt that used the dialog metric on every side.
  const int line = container->fontMetrics().height();

  auto* filter_row = new QHBoxLayout;
  filter_row->setContentsMargins(line / 2, line / 4, line / 2, line / 4);

  auto* filter = new ExplorerFilterField{container.get()};
  filter->setObjectName(QStringLiteral("explorerFilter"));
  QObject::connect(filter, &QLineEdit::textChanged, tree,
                   [tree](const QString& text) {
                     tree->SetFilterText(text.toStdU16String());
                   });
  filter_row->addWidget(filter);

  layout->addLayout(filter_row);
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
  // No root row (Tree's default): every pane built on this view is a dock
  // whose title already names the tree's root, so the row repeated it. Against
  // the shipped nodesets the Files pane read "Файлы" in its title bar and
  // "Файлы" again on its first row, and Objects and Subsystems differed from
  // their titles only by "Все". (The screenshot fixture names the file root
  // "Файловая система", so files.png showed a milder version of the same
  // thing than an operator did.)
  // The screen mockups draw a single-subject Explorer tree with no root row:
  // docs/product/ui-mockups/screens/config-workbench.html starts the hardware
  // tree at the device groups directly under the `Hardware` pane head.
  //
  // The root stays reachable as a selection: UpdateSelection() below maps an
  // empty selection onto it, so the tree's own context menu and every
  // selection-driven command still act on the root when nothing is picked.
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
  // Named local rather than `ItemDragData{...}.Save(...)`: cppcheck 2.21
  // mis-reads that shape as an access of the moved-from `node_id`.
  ItemDragData item_drag_data{std::move(node_id)};
  item_drag_data.Save(drag_data);
  return drag_data;
}
