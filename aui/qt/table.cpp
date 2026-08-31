
#include "aui/qt/table.h"
#include "aui/qt/image_util.h"

#include "aui/models/table_model.h"
#include "aui/qt/table_model_adapter.h"
#include "aui/qt/theme_qt.h"
#include "base/check.h"
#include "base/value_util.h"

#include <QAction>
#include <QClipboard>
#include <QEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QPalette>
#include <QSortFilterProxyModel>

namespace scada::aui {

namespace {

class TableProxyModel : public QSortFilterProxyModel {
 public:
  TableProxyModel(TableModel& model, const std::vector<TableColumn>& columns)
      : model_{model}, columns_{columns} {}

 protected:
  virtual bool lessThan(const QModelIndex& source_left,
                        const QModelIndex& source_right) const override;

 private:
  TableModel& model_;
  const std::vector<TableColumn>& columns_;
};

bool TableProxyModel::lessThan(const QModelIndex& source_left,
                               const QModelIndex& source_right) const {
  base::Check(source_left.column() == source_right.column());
  int column_id = columns_[source_left.column()].id;
  return model_.CompareCells(source_left.row(), source_right.row(), column_id) <
         0;
}

// The default width for a column: the configured width, tuned for the UI
// font — widened for value/timestamp columns when the token theme renders
// them in the monospace value font, scaling by the two fonts' advance over a
// digit-heavy sample so timestamps keep fitting instead of eliding
// (design-language.md §3 tabular numerals). A user-saved window state still
// overrides this via RestoreState.
int DefaultColumnWidth(const TableColumn& column) {
  if (!column.monospace && column.data_type != TableColumn::DataType::DateTime)
    return column.width;
  const QString sample = QStringLiteral("00.00.0000 00:00:00.000");
  const int ui_advance =
      QFontMetrics{QGuiApplication::font()}.horizontalAdvance(sample);
  const int mono_advance =
      QFontMetrics{MonoValueFont()}.horizontalAdvance(sample);
  if (ui_advance <= 0 || mono_advance <= ui_advance)
    return column.width;
  return column.width * mono_advance / ui_advance;
}

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

Table::Table(std::shared_ptr<TableModel> model,
             std::vector<TableColumn> columns,
             bool sorting)
    : model_adapter_{std::make_unique<TableModelAdapter>(std::move(model),
                                                         std::move(columns))} {
  horizontalHeader()->setHighlightSections(false);
  verticalHeader()->setDefaultSectionSize(19);

  if (sorting) {
    proxy_model_ = std::make_unique<TableProxyModel>(model_adapter_->model(),
                                                     model_adapter_->columns());
    proxy_model_->setSourceModel(model_adapter_.get());
    proxy_model_->setDynamicSortFilter(true);
    setModel(proxy_model_.get());
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
  } else {
    setModel(model_adapter_.get());
  }

  for (int i = 0; i < static_cast<int>(model_adapter_->columns().size()); ++i)
    setColumnWidth(i, DefaultColumnWidth(model_adapter_->columns()[i]));

  setWordWrap(false);
  setShowGrid(false);
  setSelectionBehavior(SelectRows);
  ApplyThemePalette();

  // Choosing columns is a header right-click on every desktop table an
  // operator already uses; there is no reason to invent a different affordance.
  horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(horizontalHeader(), &QWidget::customContextMenuRequested, this,
          [this](const QPoint& position) { ShowColumnMenu(position); });

  connect(horizontalHeader(), &QHeaderView::sectionResized,
          [this](int index, int old_size, int new_size) {
            // Hiding a section reports a resize to zero. Recording that would
            // destroy the column's width, so it comes back as an ungrabbable
            // sliver — and SaveState would persist the zero. Only a real
            // resize updates the stored width.
            if (new_size > 0)
              model_adapter_->columns()[index].width = new_size;
          });
}

Table::~Table() {
  setModel(nullptr);
}

void Table::LoadGlyphs(std::span<const std::string_view> resource_paths,
                       int size) {
  model_adapter_->LoadGlyphs(resource_paths, size, GlyphTintFor(palette()),
                             devicePixelRatioF());
}

const std::vector<TableColumn>& Table::columns() const {
  return model_adapter_->columns();
}

void Table::SetSelectionChangeHandler(SelectionChangeHandler handler) {
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, handler);
}

void Table::SetContextMenuHandler(ContextMenuHandler handler) {
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested,
          [this, handler](const QPoint& pos) {
            handler(viewport()->mapToGlobal(pos));
          });
}

void Table::SetKeyPressHandler(KeyPressHandler handler) {
  key_press_handler_ = std::move(handler);
}

void Table::ApplyThemePalette() {
  QPalette themed_palette = palette();
  SetDefaultItemColors(themed_palette);
  if (themed_palette != palette())
    setPalette(themed_palette);
}

std::vector<int> Table::GetSelectedRows() const {
  std::vector<int> rows;
  if (selectionModel()) {
    auto indexes = selectionModel()->selectedRows();
    rows.reserve(indexes.size());
    for (const auto& index : indexes)
      rows.emplace_back(IndexToRow(index));
  }
  return rows;
}

void Table::SelectRow(int row, bool make_visible) {
  selectRow(RowToIndex(row).row());
}

void Table::OpenEditor(int row) {
  edit(RowToIndex(row));
}

boost::json::value Table::SaveState() const {
  boost::json::value data{boost::json::object{}};
  auto& header = *horizontalHeader();
  // Hoisted: the local `columns` below shadows the columns() accessor.
  const std::vector<TableColumn>& table_columns = columns();
  boost::json::array columns;
  for (int i = 0;; ++i) {
    int index = header.logicalIndex(i);
    if (index == -1)
      break;
    boost::json::value column{boost::json::object{}};
    SetKey(column, "ix", index);
    // A hidden section reports size 0, which would restore as a zero-width
    // column the operator cannot grab. Record the size it will have when shown
    // again, and carry the hidden flag separately.
    const bool hidden = header.isSectionHidden(index);
    SetKey(column, "size",
           hidden ? DefaultColumnWidth(table_columns[index])
                  : header.sectionSize(index));
    if (hidden)
      SetKey(column, "hidden", 1);
    columns.emplace_back(std::move(column));
  }
  data.as_object()["columns"] = std::move(columns);
  return data;
}

void Table::RestoreState(const boost::json::value& data) {
  if (auto* columns = GetList(data, "columns")) {
    auto& header = *horizontalHeader();
    int visual_index = 0;
    for (auto& column : *columns) {
      int index = GetInt(column, "ix");
      int size = GetInt(column, "size");
      header.resizeSection(index, size);
      header.setSectionHidden(index, GetInt(column, "hidden") != 0);
      header.swapSections(header.visualIndex(index), visual_index);
      ++visual_index;
    }
    for (; visual_index < header.count(); ++visual_index)
      header.hideSection(header.logicalIndex(visual_index));
  }
}

int Table::ColumnSection(int column_id) const {
  const std::vector<TableColumn>& table_columns = columns();
  for (size_t i = 0; i < table_columns.size(); ++i) {
    if (table_columns[i].id == column_id)
      return static_cast<int>(i);
  }
  return -1;
}

bool Table::IsColumnVisible(int column_id) const {
  const int section = ColumnSection(column_id);
  return section != -1 && !horizontalHeader()->isSectionHidden(section);
}

void Table::SetColumnVisible(int column_id, bool visible) {
  const int section = ColumnSection(column_id);
  if (section == -1)
    return;

  // Refuse to hide the last one. The header context menu is the only way to
  // bring a column back, and a header with no sections has nothing to
  // right-click — the table would be unrecoverable without editing the
  // profile by hand.
  if (!visible && VisibleColumnCount() <= 1)
    return;

  QHeaderView& header = *horizontalHeader();
  header.setSectionHidden(section, !visible);

  // A section can come back at zero width — Qt restores the size it had when
  // hidden, and a column hidden before it was ever laid out (restored from a
  // profile, say) had none. A zero-width column is invisible and too thin to
  // grab, so it would look like the show had failed.
  if (visible && header.sectionSize(section) == 0)
    header.resizeSection(section, DefaultColumnWidth(columns()[section]));
}

int Table::VisibleColumnCount() const {
  const QHeaderView& header = *horizontalHeader();
  int visible = 0;
  for (int section = 0; section < header.count(); ++section) {
    if (!header.isSectionHidden(section))
      ++visible;
  }
  return visible;
}

void Table::ShowColumnMenu(const QPoint& position) {
  QMenu menu;
  const std::vector<TableColumn>& table_columns = columns();
  const bool last_one_left = VisibleColumnCount() <= 1;

  for (const TableColumn& column : table_columns) {
    QAction* action = menu.addAction(QString::fromStdU16String(column.title));
    action->setCheckable(true);
    const bool visible = IsColumnVisible(column.id);
    action->setChecked(visible);
    // The last visible column stays checked and disabled rather than silently
    // ignoring the click, so the refusal is visible instead of mysterious.
    action->setEnabled(!visible || !last_one_left);
    const int column_id = column.id;
    connect(action, &QAction::toggled, this, [this, column_id](bool checked) {
      SetColumnVisible(column_id, checked);
    });
  }

  menu.exec(horizontalHeader()->mapToGlobal(position));
}

QModelIndex Table::RowToIndex(int row) const {
  auto index = model_adapter_->index(row, 0);
  if (proxy_model_)
    index = proxy_model_->mapFromSource(index);
  return index;
}

int Table::IndexToRow(const QModelIndex& index) const {
  auto index2 = index;
  if (proxy_model_)
    index2 = proxy_model_->mapToSource(index);
  return index2.row();
}

void Table::SetDoubleClickHandler(DoubleClickHandler handler) {
  QObject::connect(this, &Table::doubleClicked, handler);
}

void Table::changeEvent(QEvent* event) {
  QTableView::changeEvent(event);

  switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
      ApplyThemePalette();
      // The glyphs are rendered in a palette colour, so a theme change has to
      // re-render them; an SVG icon cannot be recoloured after the fact.
      model_adapter_->RetintGlyphs(GlyphTintFor(palette()),
                                   devicePixelRatioF());
      break;
    default:
      break;
  }
}

void Table::keyPressEvent(QKeyEvent* event) {
  if (key_press_handler_ &&
      key_press_handler_(static_cast<KeyCode>(event->key())))
    return;

  if (event->matches(QKeySequence::Copy)) {
    CopyToClipbard();
    return;
  }

  QTableView::keyPressEvent(event);
}

void Table::SetStateChangeHandler(StateChangeHandler handler) {
  connect(horizontalHeader(), &QHeaderView::sectionResized, handler);
}

void Table::CopyToClipbard() {
  QModelIndexList indexes;
  for (int row : GetSelectedRows())
    indexes.push_back(RowToIndex(row));
  auto* mime_data = model()->mimeData(indexes);
  QGuiApplication::clipboard()->setMimeData(mime_data);
}

}  // namespace scada::aui
