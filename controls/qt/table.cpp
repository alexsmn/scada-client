#include "table.h"

#include "value_util.h"
#include "window_definition_util.h"

#include <QKeyEvent>

Table::Table(ui::TableModel& model,
             std::vector<ui::TableColumn> columns,
             bool sorting)
    : model_adapter_(model, std::move(columns)) {
  horizontalHeader()->setHighlightSections(false);
  verticalHeader()->setDefaultSectionSize(19);

  if (sorting) {
    proxy_model_ = std::make_unique<QSortFilterProxyModel>();
    proxy_model_->setSourceModel(&model_adapter_);
    proxy_model_->setDynamicSortFilter(true);
    setModel(proxy_model_.get());
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
  } else {
    setModel(&model_adapter_);
  }

  for (int i = 0; i < static_cast<int>(model_adapter_.columns().size()); ++i)
    setColumnWidth(i, model_adapter_.columns()[i].width);

  setShowGrid(false);
  setSelectionBehavior(SelectRows);
  connect(horizontalHeader(), &QHeaderView::sectionResized,
          [this](int index, int old_size, int new_size) {
            model_adapter_.columns()[index].width = new_size;
          });
}

Table::~Table() {
  setModel(nullptr);
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

base::Value Table::SaveState() const {
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

void Table::RestoreState(const base::Value& data) {
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

QModelIndex Table::RowToIndex(int row) const {
  auto index = model_adapter_.index(row, 0);
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

void Table::keyPressEvent(QKeyEvent* event) {
  if (key_press_handler_ &&
      key_press_handler_(static_cast<KeyCode>(event->key())))
    return;

  QTableView::keyPressEvent(event);
}
