#include "controls/wt/tree.h"

#include "controls/color.h"
#include "controls/wt/tree_model_adapter.h"
#include "value_util.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <Wt/WPainter.h>
#pragma warning(pop)

// TreeProxyModel

void TreeProxyModel::SetCompareHandler(TreeCompareHandler handler) {
  compare_handler_ = std::move(handler);
  invalidate();
}

bool TreeProxyModel::lessThan(const Wt::WModelIndex& source_left,
                              const Wt::WModelIndex& source_right) const {
  assert(source_left.column() == source_right.column());

  if (compare_handler_ && source_left.column() == 0) {
    return compare_handler_(tree_.model_adapter_->GetNode(source_left),
                            tree_.model_adapter_->GetNode(source_right)) < 0;
  }

  return WSortFilterProxyModel::lessThan(source_left, source_right);
}

// Tree

Tree::Tree(std::shared_ptr<ui::TreeModel> model)
    : model_adapter_{std::make_shared<TreeModelAdapter>(model)},
      proxy_model_{std::make_shared<TreeProxyModel>(*this)} {
  setSelectionMode(Wt::SelectionMode::Single);

  setItemDelegate(std::make_shared<ItemDelegate>(
      [this, model](const Wt::WModelIndex& index) {
        auto source_index = proxy_model_->mapToSource(index);
        return model->GetEditData(source_index.internalPointer(),
                                  source_index.column());
      }));

  proxy_model_->setDynamicSortFilter(true);
  proxy_model_->setSourceModel(model_adapter_);
  setModel(proxy_model_);

  SetRootVisible(false);

  // https://stackoverflow.com/questions/26011291/initial-width-of-column-in-qtableview-via-model
  // If you need to initialize column widths based on Qt::SizeHintRole you need
  // to:
  // - inherit your class from QTableView;
  // - reimplement method setModel and use and set initial widths of columns
  // based on Qt::SizeHintRole using method QTableView::setColumnWidth.
  for (int i = 0; i < this->model()->columnCount(); ++i) {
    int width = model->GetColumnPreferredSize(i);
    if (width != 0)
      setColumnWidth(i, width);
  }

  expand(this->model()->index(0, 0));
}

Tree::~Tree() {
  // NOTE: Can't set to nullptr.
  // setModel(nullptr);
  // setItemDelegate(nullptr);
}

void Tree::SetSorted(bool sorted) {
  setSortingEnabled(sorted);
  if (sorted)
    sortByColumn(0, Wt::SortOrder::Ascending);
}

void Tree::LoadIcons(unsigned resource_id, int width, UiColor mask_color) {
  // model_adapter_->LoadIcons(resource_id, width, ToQColor(mask_color));
}

void Tree::SelectNode(void* node) {
  select(GetIndex(node, 0), Wt::SelectionFlag::ClearAndSelect);
}

int Tree::GetSelectionSize() const {
  return selectedIndexes().size();
}

void* Tree::GetSelectedNode() {
  auto indexes = selectedIndexes();
  if (indexes.size() != 1)
    return nullptr;
  return GetNode(*indexes.begin());
}

bool Tree::IsExpanded(void* node, bool up_to_root) const {
  return isExpanded(GetIndex(node, 0));
}

void Tree::SetExpandedHandler(TreeExpandedHandler handler) {
  expanded().connect([this, handler](const Wt::WModelIndex& index) {
    handler(GetNode(index), true);
  });
  collapsed().connect([this, handler](const Wt::WModelIndex& index) {
    handler(GetNode(index), false);
  });
}

void Tree::StartEditing(void* node) {
  edit(proxy_model_->mapFromSource(model_adapter_->GetNodeIndex(node, 0)));
}

void Tree::SetDoubleClickHandler(DoubleClickHandler handler) {
  doubleClicked().connect(handler);
}

void Tree::SetSelectionChangedHandler(SelectionChangedHandler handler) {
  selectionChanged().connect(handler);
}

void Tree::SetShowChecks(bool show) {
  model_adapter_->SetCheckable(true);
}

void Tree::SetCheckedHandler(TreeCheckedHandler handler) {
  model_adapter_->SetCheckedHandler(std::move(handler));
}

bool Tree::IsChecked(void* node) const {
  return model_adapter_->IsChecked(node);
}

void Tree::SetChecked(void* node, bool checked) {
  model_adapter_->SetChecked(node, checked);
}

void Tree::SetCheckedNodes(std::set<void*> nodes) {
  model_adapter_->SetCheckedNodes(std::move(nodes));
}

void Tree::SetRootVisible(bool visible) {
  /*if (visible)
    setRootIndex({});
  else
    setRootIndex(model()->index(0, 0));
  setRootIsDecorated(!visible);*/
}

void Tree::SetCompareHandler(TreeCompareHandler handler) {
  proxy_model_->SetCompareHandler(std::move(handler));
}

void Tree::SetContextMenuHandler(ContextMenuHandler handler) {
  /*setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          [this, handler](const QPoint& pos) {
            handler(viewport()->mapToGlobal(pos));
          });*/
}

std::vector<void*> Tree::GetOrderedNodes(void* root, bool checked) const {
  struct Helper {
    void Traverse(const Wt::WModelIndex& index) {
      auto* node = tree.GetNode(index);
      if (tree.model_adapter_->IsChecked(node) != checked)
        return;
      nodes.emplace_back(node);
      for (int i = 0; i < tree.proxy_model_->rowCount(index); ++i)
        Traverse(tree.proxy_model_->index(i, 0, index));
    }

    const Tree& tree;
    const bool checked;
    std::vector<void*> nodes;
  };

  Helper helper{*this, checked};
  helper.Traverse(GetIndex(root, 0));
  return std::move(helper.nodes);
}

void Tree::SetHeaderVisible(bool visible) {
  // setHeaderHidden(!visible);
}

void* Tree::GetNode(const Wt::WModelIndex& index) const {
  return model_adapter_->GetNode(proxy_model_->mapToSource(index));
}

Wt::WModelIndex Tree::GetIndex(void* node, int column_id) const {
  return proxy_model_->mapFromSource(
      model_adapter_->GetNodeIndex(node, column_id));
}

/*void Tree::drawBranches(QPainter* painter,
                        const QRect& rect,
                        const Wt::WModelIndex& index) const {
  const auto& brush =
      proxy_model_->data(index, Qt::BackgroundRole).value<QBrush>();
  if (brush != Qt::NoBrush)
    painter->fillRect(rect, brush);

  QTreeView::drawBranches(painter, rect, index);
}*/

void Tree::SetRowHeight(int row_height) {
  model_adapter_->row_height = row_height;
}

base::Value Tree::SaveState() const {
  base::Value data{base::Value::Type::DICTIONARY};
  /*auto& header = *this->header();
  base::ListValue columns;
  for (int i = 0;; ++i) {
    int index = header.logicalIndex(i);
    if (index == -1)
      break;
    base::DictionaryValue column;
    column.SetInteger("ix", index);
    column.SetInteger("size", header.sectionSize(index));
    columns.GetList().emplace_back(std::move(column));
  }
  data.SetKey("columns", std::move(columns));*/
  return data;
}

void Tree::RestoreState(const base::Value& data) {
  /*if (auto* columns = GetList(data, "columns")) {
    auto& header = *this->header();
    int visual_index = 0;
    for (auto& column : *columns) {
      int index = GetInt(column, "ix");
      int size = GetInt(column, "size");
      header.resizeSection(index, size);
      header.swapSections(header.visualIndex(index), visual_index);
      ++visual_index;
    }
    for (; visual_index < header.count(); ++visual_index)
      header.hideSection(header.logicalIndex(visual_index));
  }*/
}

void Tree::SetFocusHandler(FocusHandler handler) {
  focus_handler_ = handler;

  clicked().connect(std::move(handler));
}
