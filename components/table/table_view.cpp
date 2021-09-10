#include "components/table/table_view.h"

#include "client_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "components/table/table_model.h"
#include "components/table/table_row.h"
#include "contents_observer.h"
#include "controller_delegate.h"
#include "controls/table.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "services/profile.h"

// TableView

TableView::TableView(const ControllerContext& context)
    : ControllerContext{context},
      model_{std::make_shared<TableModel>(
          TableModelContext{executor_, timed_data_service_, event_fetcher_,
                            profile_, dialog_service_, blinker_manager_})} {
  model_->item_changed_ = [this](const scada::NodeId& item_id, bool added) {
    NotifyContainedItemChanged(item_id, added);
  };

  std::vector<ui::TableColumn> columns = {
      {TableModel::COLUMN_TITLE, L"Имя", 150, ui::TableColumn::LEFT},
      {TableModel::COLUMN_VALUE, L"Значение", 100, ui::TableColumn::RIGHT},
      {TableModel::COLUMN_CHANGE_TIME, L"Время изменения", 170,
       ui::TableColumn::LEFT, ui::TableColumn::DataType::DateTime},
      {TableModel::COLUMN_UPDATE_TIME, L"Время обновления", 170,
       ui::TableColumn::LEFT, ui::TableColumn::DataType::DateTime},
      {TableModel::COLUMN_EVENT, L"Событие", 200, ui::TableColumn::LEFT},
  };

  view_ = new Table{model_, std::move(columns)};

  view_->SetSelectionChangeHandler([this] { OnSelectionChanged(); });

  view_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_TABLE_POPUP, point, true);
  });

  view_->SetDoubleClickHandler([this] { OnDoubleClick(); });

  view_->SetKeyPressHandler(
      [this](KeyCode key_code) { return OnKeyPressed(key_code); });

  selection_.multiple_handler = [this] { return GetMultipleSelection(); };

  delete_command_.execute_handler = [this] {
    view_->CloseEditor();
    DeleteSelection();
  };

  rename_command_.execute_handler = [this] {
    view_->OpenEditor(view_->GetCurrentRow());
  };

  move_up_command_.execute_handler = [this] { MoveRow(true); };
  move_down_command_.execute_handler = [this] { MoveRow(false); };

  sort_name_command_.execute_handler = [this] { model_->Sort(ID_SORT_NAME); };
  sort_channel_command_.execute_handler = [this] {
    model_->Sort(ID_SORT_CHANNEL);
  };
}

TableView::~TableView() {}

/*bool TableView::OnDrawCell(views::TableView& sender,
                           gfx::Canvas* canvas,
                           int row,
                           int col,
                           const gfx::Rect& rect) {
  if (view_->editing() && view_->selection_model().IsSelected(row))
    return true;

  static SkColor cell_color =
      skia::COLORREFToSkColor(::GetSysColor(COLOR_WINDOW));
  static SkColor text_color =
      skia::COLORREFToSkColor(::GetSysColor(COLOR_WINDOWTEXT));

  // get cell
  TableModel::CellEx cell;
  cell.row = row;
  cell.column_id = sender.visible_columns()[col].column.id;
  cell.cell_color = cell_color;
  cell.text_color = text_color;
  cell.image_index = -1;
  model_->GetCellEx(cell);

  if (view_->selection_model().IsSelected(row) && view_->HasFocus()) {
    static const SkColor kSelectionCellColor =
        skia::COLORREFToSkColor(::GetSysColor(COLOR_HIGHLIGHT));
    static const SkColor kSelectionTextColor =
        skia::COLORREFToSkColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
    cell.cell_color = kSelectionCellColor;
    cell.text_color = kSelectionTextColor;
  }

  unsigned text_format = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
  switch (cell.column_id) {
    case ui::TableColumn::RIGHT:
      text_format |= DT_RIGHT;
      break;
    case ui::TableColumn::CENTER:
      text_format |= DT_CENTER;
      break;
    case ui::TableColumn::LEFT:
    default:
      text_format |= DT_LEFT;
      break;
  }

  WTL::CDCHandle dc(canvas->native_canvas());

  // Draw item.
  canvas->FillRect(rect, cell.cell_color);

  bool new_row = (row == model_->row_count()) && (col == 0);

  // Draw text
  if (!cell.text.empty() || new_row) {
    RECT rect2 = rect.ToRECT();
    InflateRect(&rect2, -4, -2);

    if (cell.image_index != -1) {
      SIZE size;
      image_list_->GetIconSize(size);
      image_list_->Draw(dc, cell.image_index, rect2.left,
                        (rect2.top + rect2.bottom - size.cy) / 2,
                        ILD_TRANSPARENT);
      rect2.left += size.cx + 3;
    }

    const gfx::Font* font = &sender.font();
    if (new_row) {
      font = &new_row_font_;
      if (cell.column_id == TableModel::COLUMN_TITLE) {
        cell.text = L"Введите выражение для добавления строки";
        cell.text_color = profile_.bad_value_color;
      }
    }

    int save = dc.SaveDC();
    RECT tmp_rect = rect.ToRECT();
    dc.IntersectClipRect(&tmp_rect);
    if (col == 1) {
      DrawColoredString(canvas, *font, cell.text_color, rect2, cell.text,
                        text_format);
    } else {
      canvas->DrawString(cell.text, *font, cell.text_color, gfx::Rect(rect2),
                         text_format);
    }
    dc.RestoreDC(save);
  }

  return true;
}

views::ComboTextfield* TableView::OnCreateEditor(views::TableView& sender,
                                                 int row,
                                                 int column_id) {
  assert(column_id == 0);

  views::ComboTextfield* editor =
      __super::OnCreateEditor(sender, row, column_id);

  const TableRow* r = model_->GetRow(row);
  if (r)
    editor->SetText(base::SysNativeMBToWide(r->GetFormula()));

  return editor;
}*/

UiView* TableView::Init(const WindowDefinition& definition) {
  for (auto& item : definition.items) {
    if (item.name_is("State")) {
      view_->RestoreState(item.attributes);

    } else if (item.name_is("Item")) {
      int ix = item.GetInt("ix", 0) - 1;
      if (ix == -1)
        ix = model_->row_count();
      auto path = item.GetString("path");
      model_->SetFormula(ix, std::string{path});
    }
  }

  return view_->CreateParentIfNecessary();
}

void TableView::Save(WindowDefinition& definition) {
  definition.AddItem("State").attributes = view_->SaveState();

  for (int i = 0; i < model_->row_count(); i++) {
    TableRow* row = model_->GetRow(i);
    if (!row)
      continue;

    auto formula = row->GetFormula();
    if (formula.empty())
      continue;

    WindowItem& item = definition.AddItem("Item");
    item.SetInt("ix", i + 1);
    // WARNING: |SetString()| argument mustn't be an xvalue.
    item.SetString("path", formula);
  }
}

bool TableView::OnKeyPressed(KeyCode key_code) {
  switch (key_code) {
    case KeyCode::Enter:
      if (!view_->editing()) {
        OnDoubleClick();
        return true;
      }
      break;

    case KeyCode::Delete:
      if (!view_->editing()) {
        DeleteSelection();
        return true;
      }
      break;

    case KeyCode::Up:
    case KeyCode::Down:
      if (GetAsyncKeyState(VK_CONTROL) < 0) {
        MoveRow(key_code == KeyCode::Up);
        return true;
      }
      break;
  }

  return false;
}

void TableView::OnDoubleClick() {
  int index = view_->GetCurrentRow();
  if (index < 0)
    return;

  auto* row = model_->GetRow(index);
  if (!row)
    return;

  if (row->is_blinking()) {
    row->timed_data().Acknowledge();

  } else {
    const auto& node = row->timed_data().GetNode();
    if (node)
      controller_delegate_.ExecuteDefaultNodeCommand(node);
  }
}

void TableView::AddContainedItem(const scada::NodeId& node_id, unsigned flags) {
  if (!(flags & APPEND))
    model_->Clear();

  auto node = node_service_.GetNode(node_id);
  if (IsInstanceOf(node, data_items::id::DataGroupType)) {
    for (auto& child : node.targets(scada::id::HasComponent))
      AddContainedItem(child.node_id(), flags | APPEND);
    return;
  }

  if (node.node_class() != scada::NodeClass::Variable)
    return;

  int ix = model_->FindItem(node_id);
  if (ix != -1) {
    view_->SelectRow(ix, true);
    return;
  }

  ix = model_->row_count();
  model_->SetFormula(ix, MakeNodeIdFormula(node_id));

  view_->SelectRow(ix, true);

  controller_delegate_.SetModified(true);
}

void TableView::RemoveContainedItem(const scada::NodeId& node_id) {
  int ix;
  while ((ix = model_->FindItem(node_id)) >= 0)
    model_->DeleteRows(ix, 1);
}

void TableView::DeleteSelection() {
  for (int row : view_->GetSelectedRows())
    model_->DeleteRows(row, 1);
}

NodeIdSet TableView::GetMultipleSelection() {
  NodeIdSet node_ids;
  for (auto row_index : view_->GetSelectedRows()) {
    auto* row = model_->GetRow(row_index);
    const auto& node_id = row->timed_data().GetNode().node_id();
    if (!node_id.is_null())
      node_ids.emplace(node_id);
  }
  return node_ids;
}

NodeIdSet TableView::GetContainedItems() const {
  NodeIdSet items;
  for (int i = 0; i < model_->row_count(); i++) {
    const TableRow* row = model_->GetRow(i);
    if (!row)
      continue;
    auto node_id = row->timed_data().GetNode().node_id();
    if (!node_id.is_null())
      items.emplace(std::move(node_id));
  }
  return items;
}

CommandHandler* TableView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

void TableView::MoveRow(bool up) {
  int row = view_->GetCurrentRow();
  if (row == -1)
    return;

  auto row2 = model_->MoveRow(row, up);
  if (row2 != -1)
    view_->SelectRow(row2, true);
}

void TableView::OnSelectionChanged() {
  auto rows = view_->GetSelectedRows();
  if (rows.empty()) {
    selection_.Clear();
  } else if (rows.size() == 1) {
    if (auto* row = model_->GetRow(rows.front()))
      selection_.SelectTimedData(row->timed_data());
    else
      selection_.Clear();
  } else {
    selection_.SelectMultiple();
  }
}

ExportModel::ExportData TableView::GetExportData() {
  return TableExportData{*model_, view_->columns(),
                         Range{0, model_->row_count()}};
}
