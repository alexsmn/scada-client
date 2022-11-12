#include "controls/wt/table_model_adapter.h"

#include "controls/color.h"
#include "controls/models/table_model.h"

TableModelAdapter::TableModelAdapter(std::shared_ptr<aui::TableModel> model,
                                     std::vector<aui::TableColumn> columns)
    : model_{std::move(model)}, columns_(std::move(columns)) {
  model_->observers().AddObserver(this);
}

TableModelAdapter::~TableModelAdapter() {
  model_->observers().RemoveObserver(this);
}

int TableModelAdapter::rowCount(const Wt::WModelIndex& parent) const {
  return model_->GetRowCount();
}

int TableModelAdapter::columnCount(const Wt::WModelIndex& parent) const {
  return static_cast<int>(columns_.size());
}

Wt::cpp17::any TableModelAdapter::data(const Wt::WModelIndex& index,
                                       Wt::ItemDataRole role) const {
  auto& column = columns_[index.column()];

  aui::TableCell cell;
  cell.row = index.row();
  cell.column_id = column.id;
  model_->GetCell(cell);

  switch (role.value()) {
    case Wt::ItemDataRole::Display:
    case Wt::ItemDataRole::Edit:
      return Wt::WString{cell.text};
    default:
      return Wt::cpp17::any();
  }
}

bool TableModelAdapter::setData(const Wt::WModelIndex& index,
                                const Wt::cpp17::any& value,
                                Wt::ItemDataRole role) {
  switch (role.value()) {
    case Wt::ItemDataRole::Edit:
      return model_->SetCellText(index.row(), columns_[index.column()].id,
                                 Wt::cpp17::any_cast<Wt::WString>(value));

    default:
      return false;
  }
}

Wt::cpp17::any TableModelAdapter::headerData(int section,
                                             Wt::Orientation orientation,
                                             Wt::ItemDataRole role) const {
  if (orientation != Wt::Orientation::Horizontal)
    return Wt::cpp17::any();

  auto& column = columns_[section];

  switch (role.value()) {
    case Wt::ItemDataRole::Display:
      return Wt::WString{column.title};
    default:
      return Wt::cpp17::any();
  }
}

Wt::WFlags<Wt::ItemFlag> TableModelAdapter::flags(
    const Wt::WModelIndex& index) const {
  auto flags = Wt::WAbstractItemModel::flags(index);
  if (model_->IsEditable(index.row(), columns_[index.column()].id))
    flags |= Wt::ItemFlag::Editable;
  return flags;
}

void TableModelAdapter::OnModelChanged() {
  layoutChanged();
}

void TableModelAdapter::OnItemsChanged(int first, int count) {
  dataChanged()(index(first, 0), index(first + count - 1,
                                       static_cast<int>(columns_.size()) - 1));
}

void TableModelAdapter::OnItemsAdding(int first, int count) {
  beginInsertRows({}, first, first + count - 1);
}

void TableModelAdapter::OnItemsAdded(int first, int count) {
  endInsertRows();
}

void TableModelAdapter::OnItemsRemoving(int first, int count) {
  beginRemoveRows({}, first, first + count - 1);
}

void TableModelAdapter::OnItemsRemoved(int first, int count) {
  endRemoveRows();
}
