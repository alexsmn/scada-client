#include "controls/qt/tree.h"

#include "base/qt/color_qt.h"
#include "qt/tree_model_adapter.h"

Tree::Tree(ui::TreeModel& model)
    : model_adapter_{model},
      item_delegate_{[this, &model](const QModelIndex& index) {
        auto source_index = proxy_model_.mapToSource(index);
        return model.GetEditData(source_index.internalPointer(),
                                 source_index.column());
      }} {
  setHeaderHidden(true);
  setItemDelegate(&item_delegate_);

  proxy_model_.setDynamicSortFilter(true);
  proxy_model_.setSourceModel(&model_adapter_);
  setModel(&proxy_model_);

  SetRootVisible(false);
  setSortingEnabled(true);
  sortByColumn(0, Qt::AscendingOrder);

  // https://stackoverflow.com/questions/26011291/initial-width-of-column-in-qtableview-via-model
  // If you need to initialize column widths based on Qt::SizeHintRole you need
  // to:
  // - inherit your class from QTableView;
  // - reimplement method setModel and use and set initial widths of columns
  // based on Qt::SizeHintRole using method QTableView::setColumnWidth.
  for (int i = 0; i < this->model()->columnCount(); ++i) {
    int width = model.GetColumnPreferredSize(i);
    if (width != 0)
      setColumnWidth(i, width);
  }

  expand(this->model()->index(0, 0));
}

Tree::~Tree() {
  setModel(nullptr);
  setItemDelegate(nullptr);
}

void Tree::LoadIcons(unsigned resource_id, int width, UiColor mask_color) {
  model_adapter_.LoadIcons(resource_id, width, ColorToQt(mask_color));
}

void Tree::SelectNode(void* node) {
  selectionModel()->select(
      proxy_model_.mapFromSource(model_adapter_.GetNodeIndex(node, 0)),
      QItemSelectionModel::ClearAndSelect);
}

int Tree::GetSelectionSize() const {
  return selectionModel()->selectedRows().size();
}

void* Tree::GetSelectedNode() {
  auto rows = selectionModel()->selectedRows();
  if (rows.size() != 1)
    return nullptr;
  return model_adapter_.GetNode(proxy_model_.mapToSource(rows.front()));
}

bool Tree::IsExpanded(void* node, bool up_to_root) const {
  return isExpanded(
      proxy_model_.mapFromSource(model_adapter_.GetNodeIndex(node, 0)));
}

void Tree::SetExpandedHandler(TreeExpandedHandler handler) {
  connect(
      this, &QTreeView::expanded, [this, handler](const QModelIndex& index) {
        handler(model_adapter_.GetNode(proxy_model_.mapToSource(index)), true);
      });
  connect(
      this, &QTreeView::collapsed, [this, handler](const QModelIndex& index) {
        handler(model_adapter_.GetNode(proxy_model_.mapToSource(index)), false);
      });
}

void Tree::StartEditing(void* node) {
  edit(proxy_model_.mapFromSource(model_adapter_.GetNodeIndex(node, 0)));
}

void Tree::SetDoubleClickHandler(DoubleClickHandler handler) {
  connect(this, &QTreeView::doubleClicked, handler);
}

void Tree::SetSelectionChangedHandler(SelectionChangedHandler handler) {
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, handler);
}

void Tree::SetShowChecks(bool show) {
  model_adapter_.SetCheckable(true);
}

void Tree::SetCheckedHandler(TreeCheckedHandler handler) {
  model_adapter_.SetCheckedHandler(std::move(handler));
}

bool Tree::IsChecked(void* node) const {
  return model_adapter_.IsChecked(node);
}

void Tree::SetChecked(void* node, bool checked) {
  model_adapter_.SetChecked(node, checked);
}

void Tree::SetCheckedNodes(std::set<void*> nodes) {
  model_adapter_.SetCheckedNodes(std::move(nodes));
}

void Tree::SetRootVisible(bool visible) {
  if (visible)
    setRootIndex({});
  else
    setRootIndex(model()->index(0, 0));
  setRootIsDecorated(!visible);
}

void Tree::SetCompareHandler(TreeCompareHandler handler) {}

void Tree::SetContextMenuHandler(ContextMenuHandler handler) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          [this, handler](const QPoint& pos) {
            handler(viewport()->mapToGlobal(pos));
          });
}

std::vector<void*> Tree::GetOrderedNodes(void* root, bool checked) const {
  struct Helper {
    void Traverse(const QModelIndex& proxy_index) {
      auto* node = tree.model_adapter_.GetNode(
          tree.proxy_model_.mapToSource(proxy_index));
      if (tree.model_adapter_.IsChecked(node) != checked)
        return;
      nodes.emplace_back(node);
      for (int i = 0; i < tree.proxy_model_.rowCount(proxy_index); ++i)
        Traverse(tree.proxy_model_.index(i, 0, proxy_index));
    }

    const Tree& tree;
    const bool checked;
    std::vector<void*> nodes;
  };

  Helper helper{*this, checked};
  helper.Traverse(
      proxy_model_.mapFromSource(model_adapter_.GetNodeIndex(root, 0)));
  return std::move(helper.nodes);
}

void Tree::SetHeaderVisible(bool visible) {
  setHeaderHidden(!visible);
}
