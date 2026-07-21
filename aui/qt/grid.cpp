
#include "aui/qt/grid.h"

#include "aui/models/grid_model_util.h"
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "base/check.h"
#include "base/value_util.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>

namespace scada::aui {

namespace {

const Qt::GlobalColor kSelectionRectColor = Qt::black;
const int kSelectionRectWidth = 3;

const Qt::GlobalColor kExpandRectColor = Qt::blue;
const int kExpandHandleSize = 5;

// The design-token set for the active reshell UX theme, or null in the legacy
// look. The grid consumes aui-owned tokens only, so this stays within aui's
// allowed dependency set.
const ThemeTokens* ReshellTokens() {
  Theme theme = Theme::kDark;
  switch (GetSeverityTheme()) {
    case SeverityTheme::kLegacy:
      return nullptr;
    case SeverityTheme::kLight:
      theme = Theme::kLight;
      break;
    case SeverityTheme::kHighContrast:
      theme = Theme::kHighContrast;
      break;
    case SeverityTheme::kDark:
      break;
  }
  return &GetThemeTokens(theme);
}

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

  // Under the reshell UX theme, adopt the design tokens for the grid chrome the
  // palette does not reach: flat header sections, hairline gridlines, and a
  // soft accent selection fill. Legacy look is untouched (tokens == null). Cell
  // content still renders through the item delegate, which this does not style.
  if (const ThemeTokens* t = ReshellTokens()) {
    setStyleSheet(QStringLiteral(
                      "QTableView{ gridline-color:%1;"
                      " selection-background-color:%2; selection-color:%3; }"
                      "QHeaderView::section{ background:%4; color:%5; border:0;"
                      " border-bottom:1px solid %1; padding:2px 6px; }"
                      "QTableCornerButton::section{ background:%4; border:0;"
                      " border-bottom:1px solid %1; }")
                      // HexArgb: these tokens carry an alpha (hairline .12,
                      // soft selection .15) that the default #RRGGBB name()
                      // would drop, rendering them opaque.
                      .arg(t->border.name(QColor::HexArgb),
                           t->accent_soft.name(QColor::HexArgb), t->fg.name(),
                           t->surface_muted.name(), t->fg_muted.name()));
  }
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

  // A visible black rect reads as a gap on the dark reshell grid; use the theme
  // accent for the selection outline + expand handle when a theme is active.
  const ThemeTokens* tokens = ReshellTokens();
  const QColor selection_color =
      tokens ? tokens->accent : QColor{kSelectionRectColor};

  const auto selection_rect = GetRangeRect(selection_range_);
  if (!selection_rect.isNull()) {
    const QPen selection_pen{selection_color,
                             static_cast<qreal>(kSelectionRectWidth)};
    painter.setPen(selection_pen);
    painter.drawRect(selection_rect);
  }

  const auto expand_handle_rect = GetExpandHandleRect(selection_rect);
  if (!expand_handle_rect.isNull())
    painter.fillRect(expand_handle_rect, selection_color);

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

  const bool ctrl_pressed =
      QGuiApplication::keyboardModifiers() & Qt::ControlModifier;
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
  base::Check(index.is_valid());
  edit(model()->index(index.row, index.column));
}

boost::json::value Grid::SaveState() const {
  boost::json::value data{boost::json::object{}};
  auto& header = *horizontalHeader();
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

void Grid::RestoreState(const boost::json::value& data) {
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

}  // namespace scada::aui
