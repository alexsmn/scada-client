#include "table.h"

#include "controls/models/table_column.h"
#include "controls/models/table_model.h"
#include "window_definition_util.h"

namespace {

class TableProxyModel : public Wt::WSortFilterProxyModel {
 public:
  TableProxyModel(aui::TableModel& model,
                  const std::vector<aui::TableColumn>& columns)
      : model_{model}, columns_{columns} {}

 protected:
  virtual bool lessThan(const Wt::WModelIndex& source_left,
                        const Wt::WModelIndex& source_right) const override;

 private:
  aui::TableModel& model_;
  const std::vector<aui::TableColumn>& columns_;
};

bool TableProxyModel::lessThan(const Wt::WModelIndex& source_left,
                               const Wt::WModelIndex& source_right) const {
  assert(source_left.column() == source_right.column());
  int column_id = columns_[source_left.column()].id;
  return model_.CompareCells(source_left.row(), source_right.row(), column_id) <
         0;
}

}  // namespace

Table::Table(std::shared_ptr<aui::TableModel> model,
             std::vector<aui::TableColumn> columns,
             bool sorting)
    : model_adapter_{std::make_shared<TableModelAdapter>(std::move(model),
                                                         std::move(columns))} {
  if (sorting) {
    proxy_model_ = std::make_shared<TableProxyModel>(model_adapter_->model(),
                                                     model_adapter_->columns());
    proxy_model_->setSourceModel(model_adapter_);
    proxy_model_->setDynamicSortFilter(true);
    setModel(proxy_model_);
    setSortingEnabled(true);
    sortByColumn(0, Wt::SortOrder::Ascending);
  } else {
    setModel(model_adapter_);
  }

  for (int i = 0; i < static_cast<int>(model_adapter_->columns().size()); ++i)
    setColumnWidth(i, model_adapter_->columns()[i].width);

  setSelectionBehavior(Wt::SelectionBehavior::Rows);
  setSelectionMode(Wt::SelectionMode::Extended);

  keyPressed().connect(this, &Table::keyPressEvent);
}

Table::~Table() {
  // This doesn't work:
  // setModel(nullptr);
}

void Table::LoadIcons(unsigned resource_id, int width, UiColor mask_color) {}

int Table::GetCurrentRow() const {
  auto indexes = selectedIndexes();
  return indexes.empty() ? -1 : indexes.begin()->row();
}

bool Table::editing() const {
  return isEditing();
};

void Table::SetSelectionChangeHandler(SelectionChangeHandler handler) {
  selectionChanged().connect(handler);
}

void Table::SetContextMenuHandler(ContextMenuHandler handler) {}

void Table::SetKeyPressHandler(KeyPressHandler handler) {
  key_press_handler_ = std::move(handler);
}

std::vector<int> Table::GetSelectedRows() const {
  std::vector<int> rows;
  if (selectionModel()) {
    auto indexes = selectedIndexes();
    rows.reserve(indexes.size());
    for (const auto& index : indexes)
      rows.emplace_back(IndexToRow(index));
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
  }
  return rows;
}

void Table::SelectRow(int row, bool make_visible) {
  select(RowToIndex(row));
}

void Table::OpenEditor(int row) {
  edit(RowToIndex(row));
}

base::Value Table::SaveState() const {
  base::Value data{base::Value::Type::DICTIONARY};
  /*auto& header = *horizontalHeader();
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

void Table::RestoreState(const base::Value& data) {
  /*if (auto* columns = GetList(data, "columns")) {
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
  }*/
}

Wt::WModelIndex Table::RowToIndex(int row) const {
  auto index = model_adapter_->index(row, 0);
  if (proxy_model_)
    index = proxy_model_->mapFromSource(index);
  return index;
}

int Table::IndexToRow(const Wt::WModelIndex& index) const {
  auto index2 = index;
  if (proxy_model_)
    index2 = proxy_model_->mapToSource(index);
  return index2.row();
}

void Table::SetDoubleClickHandler(DoubleClickHandler handler) {
  doubleClicked().connect(handler);
}

void Table::keyPressEvent(const Wt::WKeyEvent& event) {
  if (key_press_handler_)
    key_press_handler_(event.key());
}

void Table::SetStateChangeHandler(StateChangeHandler handler) {
  columnResized().connect(handler);
}
