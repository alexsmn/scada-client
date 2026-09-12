
#include "aui/qt/tree.h"
#include "aui/qt/image_util.h"

#include "aui/color.h"
#include "aui/models/tree_model.h"
#include "aui/qt/item_delegate.h"
#include "aui/qt/tree_model_adapter.h"
#include "base/check.h"
#include "base/value_util.h"

#include <QEvent>
#include <QHeaderView>
#include <QPainter>
#include <QPalette>
#include <QSortFilterProxyModel>

namespace scada::aui {

// TreeProxyModel

class TreeProxyModel : public QSortFilterProxyModel {
 public:
  explicit TreeProxyModel(Tree& tree) : tree_{tree} {}

  void SetCompareHandler(TreeCompareHandler handler);

 protected:
  // QSortFilterProxyModel
  virtual bool lessThan(const QModelIndex& source_left,
                        const QModelIndex& source_right) const override;
  virtual bool filterAcceptsRow(
      int source_row,
      const QModelIndex& source_parent) const override;

 private:
  Tree& tree_;
  TreeCompareHandler compare_handler_;
};

void TreeProxyModel::SetCompareHandler(TreeCompareHandler handler) {
  compare_handler_ = std::move(handler);
  // `invalidate()`, not `invalidateFilter()`: changing the comparator changes
  // the SORT, and Qt is explicit that the narrower call does not touch it --
  // "Invalidates the current filtering" against invalidate()'s "Invalidates the
  // current sorting and filtering"
  // (https://doc.qt.io/qt-6/qsortfilterproxymodel.html, verified 2026-09-12).
  //
  // Rows already in the model keep whatever order they were sorted into
  // otherwise, and for the Explorer that is never a harmless default:
  // ConfigurationTreeView attaches the model, calls SetSorted(true) -- which
  // sorts immediately -- and only then installs the comparator, so every row
  // resident at construction was ordered by QSortFilterProxyModel's own
  // lessThan, i.e. by DisplayRole string. Measured on the screenshot fixture:
  // 100 comparisons ran with no comparator installed, which is why devices.png
  // came out as one flat alphabetical run (Latin before Cyrillic) with its
  // variable rows among the objects instead of grouped below them, and why rows
  // arriving later -- placed by the real comparator into an array sorted by the
  // wrong one -- made the result look arbitrary rather than merely different
  // (visual_review V43).
  invalidate();
}

// The model's one top-level row is the tree's root, and it is never filtered
// out. Two reasons, and the second is a correctness one: dropping it hides the
// whole tree anyway, since every other row descends from it; and
// Tree::SetRootVisible(false) makes it the view's root index, a
// QPersistentModelIndex that a removal invalidates for good — so a filter that
// matched nothing would bring the root row back the moment it was cleared.
// Rows below it filter normally, recursion included.
bool TreeProxyModel::filterAcceptsRow(int source_row,
                                      const QModelIndex& source_parent) const {
  if (!source_parent.isValid())
    return true;
  return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool TreeProxyModel::lessThan(const QModelIndex& source_left,
                              const QModelIndex& source_right) const {
  base::Check(source_left.column() == source_right.column());

  if (compare_handler_ && source_left.column() == 0) {
    return compare_handler_(tree_.model_adapter_->GetNode(source_left),
                            tree_.model_adapter_->GetNode(source_right)) < 0;
  }

  return QSortFilterProxyModel::lessThan(source_left, source_right);
}

namespace {

void SetDefaultItemColors(QPalette& palette) {
  for (auto group :
       {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
    const auto window = palette.brush(group, QPalette::Window);
    const auto window_text = palette.brush(group, QPalette::WindowText);
    palette.setBrush(group, QPalette::Base, window);
    palette.setBrush(group, QPalette::AlternateBase, window);
    palette.setBrush(group, QPalette::Text, window_text);
  }
}

}  // namespace

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
  ApplyThemePalette();

  // Prevent from editing when double-clicked.
  setEditTriggers(QTreeView::EditTrigger::SelectedClicked);

  proxy_model_->setDynamicSortFilter(true);
  // Column-0 substring filtering for SetFilterText; recursive so an ancestor of
  // a deeper match stays visible, case-insensitive to match how operators type.
  proxy_model_->setFilterKeyColumn(0);
  proxy_model_->setFilterCaseSensitivity(Qt::CaseInsensitive);
  proxy_model_->setRecursiveFilteringEnabled(true);
  proxy_model_->setSourceModel(model_adapter_.get());
  setModel(proxy_model_.get());

  SetRootVisible(false);
  // A reset invalidates every index, the root index above among them, so it
  // has to be set again — see TreeTest.HiddenRootStaysHiddenAcrossAModelReset.
  connect(proxy_model_.get(), &QAbstractItemModel::modelReset, this,
          &Tree::ApplyRootVisible);

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

void Tree::SetFilterText(const std::u16string& text) {
  proxy_model_->setFilterFixedString(QString::fromStdU16String(text));
}

void Tree::LoadGlyphs(std::span<const std::string_view> resource_paths,
                      int size) {
  model_adapter_->LoadGlyphs(resource_paths, size, GlyphTint(),
                             devicePixelRatioF());
}

Color Tree::GlyphTint() const {
  return GlyphTintFor(palette());
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

void Tree::ExpandNode(void* node) {
  expand(GetIndex(node, 0));
}

std::vector<void*> Tree::GetChildNodes(void* parent) const {
  const auto parent_index = parent ? GetIndex(parent, 0) : QModelIndex{};
  std::vector<void*> nodes;
  nodes.reserve(model()->rowCount(parent_index));
  for (int row = 0; row < model()->rowCount(parent_index); ++row)
    nodes.emplace_back(GetNode(model()->index(row, 0, parent_index)));
  return nodes;
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

void Tree::ApplyThemePalette() {
  QPalette themed_palette = palette();
  SetDefaultItemColors(themed_palette);
  if (themed_palette != palette())
    setPalette(themed_palette);
}

void Tree::changeEvent(QEvent* event) {
  QTreeView::changeEvent(event);

  switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
      ApplyThemePalette();
      // The glyphs are rendered in a palette colour, so a theme change has to
      // re-render them; an SVG icon cannot be recoloured after the fact.
      model_adapter_->RetintGlyphs(GlyphTint(), devicePixelRatioF());
      break;
    default:
      break;
  }
}

void Tree::SetSelectionChangedHandler(SelectionChangedHandler handler) {
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, handler);
}

void Tree::SetShowChecks(bool show) {
  model_adapter_->SetCheckable(show);
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
  root_visible_ = visible;
  ApplyRootVisible();
}

void Tree::ApplyRootVisible() {
  if (root_visible_) {
    setRootIndex({});
    // A visible root still needs the branch decoration; otherwise the
    // top-level node loses its expander and only the root row is shown.
    setRootIsDecorated(true);
    expand(model()->index(0, 0));
  } else {
    // Re-read the index rather than trusting the stored one: this also runs
    // after a model reset, which invalidates the QPersistentModelIndex
    // QTreeView keeps. Leaving it invalid silently re-exposes the root row.
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

void Tree::ExpandAllWhenPopulated() {
  if (populated_connection_)
    return;

  // Rows may already be present when the model is filled synchronously, in
  // which case there is nothing to wait for.
  if (model()->rowCount(rootIndex()) != 0) {
    expandAll();
    return;
  }

  populated_connection_ = connect(model(), &QAbstractItemModel::rowsInserted,
                                  this, [this](const QModelIndex&, int, int) {
                                    expandAll();
                                    disconnect(populated_connection_);
                                    populated_connection_ = {};
                                  });
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

}  // namespace scada::aui
