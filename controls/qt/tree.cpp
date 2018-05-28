#include "controls/qt/tree.h"

#include "base/qt/color_qt.h"
#include "qt/tree_model_adapter.h"

Tree::Tree(ui::TreeModel& model) : model_adapter_(new TreeModelAdapter(model)) {
  setHeaderHidden(true);
  setModel(model_adapter_.get());

  // https://stackoverflow.com/questions/26011291/initial-width-of-column-in-qtableview-via-model
  // If you need to initialize column widths based on Qt::SizeHintRole you need
  // to:
  // - inherit your class from QTableView;
  // - reimplement method setModel and use and set initial widths of columns
  // based on Qt::SizeHintRole using method QTableView::setColumnWidth.
  for (int i = 0; i < model_adapter_->columnCount(); ++i) {
    int width = model.GetColumnPreferredSize(i);
    if (width != 0)
      setColumnWidth(i, width);
  }
}

Tree::~Tree() {}

void Tree::LoadIcons(unsigned resource_id, int width, UiColor mask_color) {
  model_adapter_->LoadIcons(resource_id, width, ColorToQt(mask_color));
}

void Tree::SelectNode(void* node) {
  selectionModel()->select(model_adapter_->GetNodeIndex(node, 0),
                           QItemSelectionModel::ClearAndSelect);
}

int Tree::GetSelectionSize() const {
  return selectionModel()->selectedRows().size();
}

void* Tree::GetSelectedNode() {
  auto rows = selectionModel()->selectedRows();
  if (rows.size() != 1)
    return nullptr;
  return model_adapter_->GetNode(rows.front());
}

bool Tree::IsExpanded(void* node, bool up_to_root) const {
  return isExpanded(model_adapter_->GetNodeIndex(node, 0));
}

void Tree::SetExpandedHandler(TreeExpandedHandler handler) {
  QObject::connect(this, &QTreeView::expanded,
                   [this, handler](const QModelIndex& index) {
                     handler(model_adapter_->GetNode(index), true);
                   });
  QObject::connect(this, &QTreeView::collapsed,
                   [this, handler](const QModelIndex& index) {
                     handler(model_adapter_->GetNode(index), false);
                   });
}

void Tree::StartEditing(void* node) {}

void Tree::SetDoubleClickHandler(DoubleClickHandler handler) {
  connect(this, &QTreeView::doubleClicked, handler);
}

void Tree::SetSelectionChangedHandler(SelectionChangedHandler handler) {
  QObject::connect(selectionModel(), &QItemSelectionModel::selectionChanged,
                   handler);
}

void Tree::SetEditHandler(TreeEditHandler handler) {}

void Tree::SetCheckedHandler(TreeCheckedHandler handler) {}

void Tree::SetRootVisible(bool visible) {}

void Tree::SetEditable(bool editable) {}

void Tree::SetCompareHandler(TreeCompareHandler handler) {}
