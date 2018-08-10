#include "controls/qt/tree.h"

#include "base/qt/color_qt.h"
#include "qt/tree_model_adapter.h"

Tree::Tree(ui::TreeModel& model)
    : model_adapter_{model}, item_delegate_{[&model](const QModelIndex& index) {
        return model.GetEditData(index.internalPointer(), index.column());
      }} {
  setHeaderHidden(true);
  setModel(&model_adapter_);
  setItemDelegate(&item_delegate_);

  // https://stackoverflow.com/questions/26011291/initial-width-of-column-in-qtableview-via-model
  // If you need to initialize column widths based on Qt::SizeHintRole you need
  // to:
  // - inherit your class from QTableView;
  // - reimplement method setModel and use and set initial widths of columns
  // based on Qt::SizeHintRole using method QTableView::setColumnWidth.
  for (int i = 0; i < model_adapter_.columnCount(); ++i) {
    int width = model.GetColumnPreferredSize(i);
    if (width != 0)
      setColumnWidth(i, width);
  }
}

Tree::~Tree() {
  setModel(nullptr);
  setItemDelegate(nullptr);
}

void Tree::LoadIcons(unsigned resource_id, int width, UiColor mask_color) {
  model_adapter_.LoadIcons(resource_id, width, ColorToQt(mask_color));
}

void Tree::SelectNode(void* node) {
  selectionModel()->select(model_adapter_.GetNodeIndex(node, 0),
                           QItemSelectionModel::ClearAndSelect);
}

int Tree::GetSelectionSize() const {
  return selectionModel()->selectedRows().size();
}

void* Tree::GetSelectedNode() {
  auto rows = selectionModel()->selectedRows();
  if (rows.size() != 1)
    return nullptr;
  return model_adapter_.GetNode(rows.front());
}

bool Tree::IsExpanded(void* node, bool up_to_root) const {
  return isExpanded(model_adapter_.GetNodeIndex(node, 0));
}

void Tree::SetExpandedHandler(TreeExpandedHandler handler) {
  connect(this, &QTreeView::expanded,
          [this, handler](const QModelIndex& index) {
            handler(model_adapter_.GetNode(index), true);
          });
  connect(this, &QTreeView::collapsed,
          [this, handler](const QModelIndex& index) {
            handler(model_adapter_.GetNode(index), false);
          });
}

void Tree::StartEditing(void* node) {
  openPersistentEditor(model_adapter_.GetNodeIndex(node, 0));
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

void Tree::SetChecked(void* node, bool checked) {
  model_adapter_.SetChecked(node, checked);
}

void Tree::SetRootVisible(bool visible) {
  setRootIsDecorated(visible);
}

void Tree::SetCompareHandler(TreeCompareHandler handler) {}

void Tree::SetContextMenuHandler(ContextMenuHandler handler) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          [this, handler](const QPoint& pos) {
            handler(viewport()->mapToGlobal(pos));
          });
}
