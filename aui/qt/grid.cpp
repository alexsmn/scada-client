#include "aui/qt/grid.h"

#include "base/value_util.h"
#include "aui/models/grid_model_util.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>

namespace aui {

namespace {

const Qt::GlobalColor kSelectionRectColor = Qt::black;
const int kSelectionRectWidth = 3;

const Qt::GlobalColor kExpandRectColor = Qt::blue;
const int kExpandHandleSize = 5;

GridRange ToUiGridRange(const QItemSelectionRange& range) {
  return GridRange::Range(range.top(), range.left(), range.height(),
                          range.width());
}

}  // namespace

Grid::Grid(std::shared_ptr<GridModel> model,
           std::shared_ptr<HeaderModel> row_model,
           std::shared_ptr<HeaderModel> column_model)
    : model_{model}, model_adapter_{model, row_model, column_model} {
  item_delegate_.set_edit_data_provider([model](const QModelIndex& index) {
    return model->GetEditData(index.row(), index.column());
  });

  item_delegate_.set_button_handler([model](const QModelIndex& index) {
    model->HandleEditButton(index.row(), index.column());
  });

  horizontalHeader()->setHighlightSections(false);
  verticalHeader()->setHighlightSections(false);
  verticalHeader()->setDefaultSectionSize(19);
  setModel(&model_adapter_);
  resizeColumnsToContents();
  setItemDelegate(&item_delegate_);
  setWordWrap(false);
}

Grid::~Grid() {
  setModel(nullptr);
  setItemDelegate(nullptr);
}

void Grid::SetExpandAllowed(bool allowed) {
  if (expand_allowed_ == allowed)
    return;

  UpdateSelectionRange();
  expand_allowed_ = allowed;
  UpdateSelectionRange();

  setMouseTracking(expand_allowed_);
}

void Grid::SetColumnHeaderVisible(bool visible) {
  horizontalHeader()->setVisible(visible);
}

void Grid::SetColumnHeaderHeight(int height) {
  horizontalHeader()->setFixedHeight(height);
}

void Grid::SetRowHeaderVisible(bool visible) {
  verticalHeader()->setVisible(visible);
}

void Grid::SetRowHeaderWidth(int width) {
  verticalHeader()->setFixedWidth(width);
}

void Grid::SetContextMenuHandler(ContextMenuHandler handler) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          [this, handler](const QPoint& pos) {
            handler(viewport()->mapToGlobal(pos));
          });
}

void Grid::mousePressEvent(QMouseEvent* event) {
  const auto selection_rect = GetRangeRect(selection_range_);
  const auto expand_handle_rect = GetExpandHandleRect(selection_rect);
  expanding_ = expand_handle_rect.contains(event->pos());
  if (expanding_)
    return;

  QTableView::mousePressEvent(event);
}

void Grid::mouseReleaseEvent(QMouseEvent* event) {
  if (expanding_) {
    Expand(selection_range_, expand_range_);
    SetExpandRange({});
    unsetCursor();
    expanding_ = false;
    return;
  }

  QTableView::mouseReleaseEvent(event);
}

void Grid::mouseMoveEvent(QMouseEvent* event) {
  if (expanding_) {
    auto index = indexAt(event->pos());
    if (index.isValid()) {
      scrollTo(index);
      SetExpandRange(CalcExpandRange(index, event->pos()));
    }
    return;
  }

  QTableView::mouseMoveEvent(event);

  const auto selection_rect = GetRangeRect(selection_range_);
  const auto expand_handle_rect = GetExpandHandleRect(selection_rect);
  if (expand_handle_rect.contains(event->pos()))
    setCursor(Qt::CrossCursor);
  else
    unsetCursor();
}

void Grid::paintEvent(QPaintEvent* e) {
  QTableView::paintEvent(e);

  QPainter painter{viewport()};

  const auto selection_rect = GetRangeRect(selection_range_);
  if (!selection_rect.isNull()) {
    const QPen selection_pen{kSelectionRectColor,
                             static_cast<qreal>(kSelectionRectWidth)};
    painter.setPen(selection_pen);
    painter.drawRect(selection_rect);
  }

  const auto expand_handle_rect = GetExpandHandleRect(selection_rect);
  if (!expand_handle_rect.isNull())
    painter.fillRect(expand_handle_rect, kSelectionRectColor);

  auto expand_rect = GetRangeRect(expand_range_);
  if (!expand_rect.isNull()) {
    const QPen expand_pen{kExpandRectColor,
                          static_cast<qreal>(kSelectionRectWidth)};
    painter.setPen(expand_pen);
    painter.drawRect(expand_rect);
  }
}

void Grid::selectionChanged(const QItemSelection& selected,
                            const QItemSelection& deselected) {
  QTableView::selectionChanged(selected, deselected);

  const auto& selection = selectionModel()->selection();
  const auto& selection_range =
      selection.size() == 1 ? selection.front() : QItemSelectionRange{};
  if (selection_range_ != selection_range) {
    UpdateSelectionRange();
    selection_range_ = selection_range;
    UpdateSelectionRange();
  }
}

void Grid::UpdateSelectionRange() {
  const auto selection_rect = GetRangeRect(selection_range_);
  if (!selection_rect.isNull()) {
    viewport()->update(selection_rect.marginsAdded(
        QMargins{kSelectionRectWidth, kSelectionRectWidth, kSelectionRectWidth,
                 kSelectionRectWidth}));
  }

  const auto expand_handle_rect = GetExpandHandleRect(selection_rect);
  if (!expand_handle_rect.isNull())
    viewport()->update(expand_handle_rect);
}

QRect Grid::GetRangeRect(const QItemSelectionRange& range) const {
  if (range.isEmpty())
    return {};

  const auto rect1 = visualRect(range.topLeft());
  const auto rect2 = visualRect(range.bottomRight());
  return rect1.united(rect2);
}

QRect Grid::GetExpandHandleRect(const QRect& selection_rect) const {
  if (!expand_allowed_)
    return {};

  if (selection_rect.isNull())
    return {};

  return QRect{selection_rect.right() - kExpandHandleSize / 2 + 1,
               selection_rect.bottom() - kExpandHandleSize / 2 + 1,
               kExpandHandleSize, kExpandHandleSize};
}

QItemSelectionRange Grid::CalcExpandRange(const QModelIndex& index,
                                          QPoint pos) const {
  if (selection_range_.isEmpty())
    return {};

  if (!model())
    return {};

  if (!index.isValid())
    return {};

  auto rect = GetRangeRect(selection_range_);
  if (rect.isNull())
    return {};

  int offset_x =
      pos.x() < rect.x() ? rect.x() - pos.x() : pos.x() - rect.right();
  int offset_y =
      pos.y() < rect.y() ? rect.y() - pos.y() : pos.y() - rect.bottom();

  int row = index.row();
  int column = index.column();
  if (offset_x > offset_y)
    row = selection_range_.bottom();
  else
    column = selection_range_.right();

  auto top_left =
      model()->index(std::min(selection_range_.top(), row),
                     std::min(selection_range_.left(), column), index.parent());
  auto bottom_right = model()->index(std::max(selection_range_.bottom(), row),
                                     std::max(selection_range_.right(), column),
                                     index.parent());

  QItemSelectionRange expand_range{top_left, bottom_right};
  if (expand_range == selection_range_)
    return {};

  return expand_range;
}

void Grid::SetExpandRange(const QItemSelectionRange& range) {
  if (expand_range_ == range)
    return;

  if (auto expand_rect = GetRangeRect(expand_range_); !expand_rect.isNull()) {
    viewport()->update(expand_rect.marginsAdded(
        QMargins{kSelectionRectWidth, kSelectionRectWidth, kSelectionRectWidth,
                 kSelectionRectWidth}));
  }

  expand_range_ = range;

  if (auto expand_rect = GetRangeRect(expand_range_); !expand_rect.isNull()) {
    viewport()->update(expand_rect.marginsAdded(
        QMargins{kSelectionRectWidth, kSelectionRectWidth, kSelectionRectWidth,
                 kSelectionRectWidth}));
  }
}

void Grid::Expand(const QItemSelectionRange& range,
                  const QItemSelectionRange& expand_range) {
  if (!range.isValid() || !expand_range.isValid())
    return;

  const bool ctrl_pressed = GetAsyncKeyState(VK_CONTROL) < 0;
  ExpandGridRange(*model_, ToUiGridRange(range), ToUiGridRange(expand_range),
                  !ctrl_pressed);
}

GridModelIndex Grid::GetCurrentIndex() const {
  auto index = currentIndex();
  return index.isValid() ? GridModelIndex{index.row(), index.column()}
                         : GridModelIndex{};
}

GridRange Grid::GetSelectionRange() const {
  if (!selection_range_.isValid() || selection_range_.isEmpty())
    return {};

  return GridRange::Range(selection_range_.top(), selection_range_.left(),
                          selection_range_.height(), selection_range_.width());
}

void Grid::SetSelectionChangeHandler(SelectionChangeHandler handler) {
  connect(QTableView::selectionModel(), &QItemSelectionModel::selectionChanged,
          handler);
}

void Grid::OpenEditor(const GridModelIndex& index) {
  assert(index.is_valid());
  edit(model()->index(index.row, index.column));
}

base::Value Grid::SaveState() const {
  base::Value data{base::Value::Type::DICTIONARY};
  auto& header = *horizontalHeader();
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
  data.SetKey("columns", std::move(columns));
  return data;
}

void Grid::RestoreState(const base::Value& data) {
  if (auto* columns = GetList(data, "columns")) {
    auto& header = *horizontalHeader();
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

void Grid::RequestFocus() {
  setFocus();
}

void Grid::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::Copy)) {
    CopyToClipboard();
    return;
  }

  QTableView::keyPressEvent(event);
}

void Grid::CopyToClipboard() {
  if (!model() || !selectionModel() || !QGuiApplication::clipboard())
    return;
  auto* mime_data = model()->mimeData(selectionModel()->selectedIndexes());
  if (mime_data)
    QGuiApplication::clipboard()->setMimeData(mime_data);
}

}  // namespace aui
