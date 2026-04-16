#include "aui/qt/tree.h"

#include "base/value_util.h"
#include "aui/color.h"
#include "aui/models/tree_model.h"
#include "aui/qt/item_delegate.h"
#include "aui/qt/tree_model_adapter.h"

#include <QHeaderView>
#include <QPainter>
#include <QSortFilterProxyModel>

namespace aui {

// TreeProxyModel

class TreeProxyModel : public QSortFilterProxyModel {
 public:
  explicit TreeProxyModel(Tree& tree) : tree_{tree} {}

  void SetCompareHandler(TreeCompareHandler handler);

 protected:
  // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& source_left,
                        const QModelIndex& source_right) const override;

 private:
  Tree& tree_;
  TreeCompareHandler compare_handler_;
};

void TreeProxyModel::SetCompareHandler(TreeCompareHandler handler) {
  compare_handler_ = std::move(handler);
  invalidateFilter();
}

bool TreeProxyModel::lessThan(const QModelIndex& source_left,
                              const QModelIndex& source_right) const {
  assert(source_left.column() == source_right.column());

  if (compare_handler_ && source_left.column() == 0) {
    return compare_handler_(tree_.model_adapter_->GetNode(source_left),
                            tree_.model_adapter_->GetNode(source_right)) < 0;
  }

  return QSortFilterProxyModel::lessThan(source_left, source_right);
}

// Tree

Tree::Tree(std::shared_ptr<TreeModel> model)
    : model_adapter_{std::make_unique<TreeModelAdapter>(model)},
      proxy_model_{std::make_unique<TreeProxyModel>(*this)},
      item_delegate_{std::make_unique<ItemDelegate>()} {
  item_delegate_->set_edit_data_provider(
      [this, model](const QModelIndex& index) {
        auto source_index = proxy_model_->mapToSource(index);
        return model->GetEditData(source_index.internalPointer(),
                                  source_index.column());
      });

  item_delegate_->set_button_handler([this, model](const QModelIndex& index) {
    auto source_index = proxy_model_->mapToSource(index);
    model->HandleEditButton(source_index.internalPointer(),
                            source_index.column());
  });

  setHeaderHidden(true);
  setItemDelegate(item_delegate_.get());

  // Prevent from editing when double-clicked.
  setEditTriggers(QTreeView::EditTrigger::SelectedClicked);

  proxy_model_->setDynamicSortFilter(true);
  proxy_model_->setSourceModel(model_adapter_.get());
  setModel(proxy_model_.get());

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
  setModel(nullptr);
  setItemDelegate(nullptr);
}

void Tree::SetSorted(bool sorted) {
  setSortingEnabled(sorted);
  if (sorted)
    sortByColumn(0, Qt::SortOrder::AscendingOrder);
}

void Tree::LoadIcons(unsigned resource_id, int width, Color mask_color) {
  model_adapter_->LoadIcons(resource_id, width, mask_color);
}

void Tree::SelectNode(void* node) {
  selectionModel()->select(GetIndex(node, 0),
                           QItemSelectionModel::ClearAndSelect);
}

int Tree::GetSelectionSize() const {
  return selectionModel()->selectedRows().size();
}

void* Tree::GetSelectedNode() {
  auto rows = selectionModel()->selectedRows();
  if (rows.size() != 1)
    return nullptr;
  return GetNode(rows.front());
}

bool Tree::IsExpanded(void* node, bool up_to_root) const {
  return isExpanded(GetIndex(node, 0));
}

void Tree::SetExpandedHandler(TreeExpandedHandler handler) {
  connect(this, &QTreeView::expanded,
          [this, handler](const QModelIndex& index) {
            handler(GetNode(index), true);
          });
  connect(this, &QTreeView::collapsed,
          [this, handler](const QModelIndex& index) {
            handler(GetNode(index), false);
          });
}

void Tree::StartEditing(void* node) {
  edit(proxy_model_->mapFromSource(model_adapter_->GetNodeIndex(node, 0)));
}

void Tree::SetDoubleClickHandler(DoubleClickHandler handler) {
  connect(this, &QTreeView::doubleClicked, handler);
}

void Tree::SetSelectionChangedHandler(SelectionChangedHandler handler) {
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, handler);
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
  if (visible) {
    setRootIndex({});
    // A visible root still needs the branch decoration; otherwise the
    // top-level node loses its expander and only the root row is shown.
    setRootIsDecorated(true);
    expand(model()->index(0, 0));
  } else {
    setRootIndex(model()->index(0, 0));
    setRootIsDecorated(true);
  }
}

void Tree::SetCompareHandler(TreeCompareHandler handler) {
  proxy_model_->SetCompareHandler(std::move(handler));
}

void Tree::SetContextMenuHandler(ContextMenuHandler handler) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          [this, handler](const QPoint& pos) {
            handler(viewport()->mapToGlobal(pos));
          });
}

std::vector<void*> Tree::GetOrderedNodes(void* root, bool checked) const {
  struct Helper {
    void Traverse(const QModelIndex& index) {
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
  setHeaderHidden(!visible);
}

void* Tree::GetNode(const QModelIndex& index) const {
  return model_adapter_->GetNode(proxy_model_->mapToSource(index));
}

QModelIndex Tree::GetIndex(void* node, int column_id) const {
  return proxy_model_->mapFromSource(
      model_adapter_->GetNodeIndex(node, column_id));
}

void Tree::drawBranches(QPainter* painter,
                        const QRect& rect,
                        const QModelIndex& index) const {
  const auto& brush =
      proxy_model_->data(index, Qt::BackgroundRole).value<QBrush>();
  if (brush != Qt::NoBrush)
    painter->fillRect(rect, brush);

  QTreeView::drawBranches(painter, rect, index);
}

void Tree::SetRowHeight(int row_height) {
  model_adapter_->row_height = row_height;
}

boost::json::value Tree::SaveState() const {
  boost::json::value data{boost::json::object{}};
  auto& header = *this->header();
  boost::json::array columns;
  for (int i = 0;; ++i) {
    int index = header.logicalIndex(i);
    if (index == -1)
      break;
    boost::json::value column{boost::json::object{}};
    SetKey(column, "ix", index);
    SetKey(column, "size", header.sectionSize(index));
    columns.emplace_back(std::move(column));
  }
  data.as_object()["columns"] = std::move(columns);
  return data;
}

void Tree::RestoreState(const boost::json::value& data) {
  if (auto* columns = GetList(data, "columns")) {
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
  }
}

void Tree::SetFocusHandler(FocusHandler handler) {}

void Tree::SetDragHandler(std::vector<std::string> mime_types,
                          DragHandler handler) {
  setDragEnabled(!mime_types.empty() && handler);

  model_adapter_->SetDragHandler(std::move(mime_types), std::move(handler));
}

void Tree::SetDropHandler(DropHandler handler) {
  viewport()->setAcceptDrops(!!handler);
  setDropIndicatorShown(!!handler);

  model_adapter_->drop_handler = std::move(handler);
}

}  // namespace aui
