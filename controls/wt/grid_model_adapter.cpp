#include "controls/wt/grid_model_adapter.h"

#include "controls/color.h"
#include "ui/base/models/grid_range.h"

namespace {

Wt::AlignmentFlag UiAligmentToWt(ui::TableColumn::Alignment alignment) {
  switch (alignment) {
    case ui::TableColumn::LEFT:
      return Wt::AlignmentFlag::Left;
    case ui::TableColumn::CENTER:
      return Wt::AlignmentFlag::Center;
    case ui::TableColumn::RIGHT:
      return Wt::AlignmentFlag::Right;
    default:
      return Wt::AlignmentFlag::Left;
  }
}

}  // namespace

GridModelAdapter::GridModelAdapter(ui::GridModel& model,
                                   ui::HeaderModel& row_model,
                                   ui::HeaderModel& column_model)
    : model_(model), row_model_(row_model), column_model_(column_model) {
  model_.observers().AddObserver(this);
  column_model_.observers().AddObserver(this);
}

GridModelAdapter::~GridModelAdapter() {
  model_.observers().RemoveObserver(this);
  column_model_.observers().RemoveObserver(this);
}

int GridModelAdapter::rowCount(const Wt::WModelIndex& parent) const {
  return row_model_.GetCount();
}

int GridModelAdapter::columnCount(const Wt::WModelIndex& parent) const {
  return column_model_.GetCount();
}

Wt::cpp17::any GridModelAdapter::data(const Wt::WModelIndex& index,
                                      Wt::ItemDataRole role) const {
  /*switch (role.value()) {
    case Wt::ItemDataRole::TextAlignmentRole:
      return column_model_.GetAlignment(index.column());
  }*/

  ui::GridCell cell;
  cell.row = index.row();
  cell.column = index.column();
  model_.GetCell(cell);

  switch (role.value()) {
    case Wt::ItemDataRole::Display:
    case Wt::ItemDataRole::Edit:
      return Wt::WString{cell.text};
    /*case Wt::ItemDataRole::TextColorRole:
      return ToQColor(cell.text_color);
    case Wt::ItemDataRole::BackgroundColorRole:
      return ToQColor(cell.cell_color);*/
    default:
      return Wt::cpp17::any();
  }
}

Wt::WFlags<Wt::ItemFlag> GridModelAdapter::flags(
    const Wt::WModelIndex& index) const {
  auto flags = Wt::WAbstractTableModel::flags(index);
  if (model_.IsEditable(index.row(), index.column()))
    flags |= Wt::ItemFlag::Editable;
  return flags;
}

Wt::cpp17::any GridModelAdapter::headerData(int section,
                                            Wt::Orientation orientation,
                                            Wt::ItemDataRole role) const {
  if (orientation == Wt::Orientation::Horizontal) {
    switch (role.value()) {
      case Wt::ItemDataRole::Display:
        return Wt::WString{column_model_.GetTitle(section)};
      /*case Wt::ItemDataRole::SizeHintRole:
        return QSize(column_model_.GetSize(section), 19);*/
      default:
        return Wt::cpp17::any();
    }

  } else if (orientation == Wt::Orientation::Vertical) {
    switch (role.value()) {
      case Wt::ItemDataRole::Display:
        return Wt::WString{row_model_.GetTitle(section)};
      default:
        return Wt::cpp17::any();
    }

  } else {
    assert(false);
    return Wt::cpp17::any();
  }
}

bool GridModelAdapter::setData(const Wt::WModelIndex& index,
                               const Wt::cpp17::any& value,
                               Wt::ItemDataRole role) {
  return model_.SetCellText(index.row(), index.column(),
                            std::any_cast<Wt::WString>(value));
}

void GridModelAdapter::OnGridModelChanged(ui::GridModel& model) {
  // resetInternalData();
  layoutChanged();
}

void GridModelAdapter::OnGridRangeChanged(ui::GridModel& model,
                                          const ui::GridRange& range) {
  dataChanged()(index(range.row(), range.column()),
                index(range.row() + range.row_count() - 1,
                      range.column() + range.column_count() - 1));
}

void GridModelAdapter::OnGridRowsAdded(ui::GridModel& model,
                                       int first,
                                       int count) {
  layoutChanged();
}

void GridModelAdapter::OnGridRowsRemoved(ui::GridModel& model,
                                         int first,
                                         int count) {
  layoutChanged();
}

void GridModelAdapter::OnModelChanged(ui::HeaderModel& model) {
  if (&model == &column_model_)
    headerDataChanged()(Wt::Orientation::Horizontal, 0, model.GetCount());
  else if (&model == &row_model_)
    headerDataChanged()(Wt::Orientation::Vertical, 0, model.GetCount());
}
