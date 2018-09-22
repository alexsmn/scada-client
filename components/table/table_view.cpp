#include "components/table/table_view.h"

#include "client_utils.h"
#include "common/formula_util.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/table/table_model.h"
#include "components/table/table_row.h"
#include "contents_observer.h"
#include "controller_factory.h"
#include "controls/table.h"
#include "print_util.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "services/profile.h"

#if defined(UI_VIEWS)
#include "base/color_string.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/table/table_painter.h"
#include "ui/views/controls/textfield/combo_textfield.h"

// TODO: Remove.
#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#endif

#if defined(UI_VIEWS)
// TableViewPainter
class TableViewPainter : public views::TablePainter {
 public:
  virtual void PaintSelectionBackground(gfx::Canvas* canvas,
                                        int index,
                                        const gfx::Rect& bounds) override {}
  virtual void PaintSelection(gfx::Canvas* canvas,
                              int index,
                              const gfx::Rect& bounds) override {}
};
#endif

// TableView

const WindowInfo kWindowInfo = {ID_TABLE_VIEW,           "Table", L"Таблица",
                                WIN_INS | WIN_CAN_PRINT, 620,     400};

REGISTER_CONTROLLER(TableView, kWindowInfo);

TableView::TableView(const ControllerContext& context) : Controller{context} {
  model_ = std::make_unique<TableModel>(TableModelContext{
      timed_data_service_, event_manager_, profile_, dialog_service_});
  model_->item_changed_ = [this](const scada::NodeId& item_id, bool added) {
    if (contents_observer())
      contents_observer()->OnContainedItemChanged(item_id, added);
  };

  const ui::TableColumn columns[] = {
      ui::TableColumn(TableModel::COLUMN_TITLE, L"Имя", 150,
                      ui::TableColumn::LEFT),
      ui::TableColumn(TableModel::COLUMN_VALUE, L"Значение", 100,
                      ui::TableColumn::RIGHT),
      ui::TableColumn(TableModel::COLUMN_CHANGE_TIME, L"Время изменения", 170,
                      ui::TableColumn::LEFT),
      ui::TableColumn(TableModel::COLUMN_UPDATE_TIME, L"Время обновления", 170,
                      ui::TableColumn::LEFT),
      ui::TableColumn(TableModel::COLUMN_EVENT, L"Событие", 200,
                      ui::TableColumn::LEFT)};

  view_ = std::make_unique<Table>(
      *model_,
      std::vector<ui::TableColumn>(columns, columns + _countof(columns)));

#if defined(UI_QT)
  QObject::connect(
      view_->selectionModel(), &QItemSelectionModel::selectionChanged,
      [this](const QItemSelection& selected, const QItemSelection& deselected) {
        OnSelectionChanged();
      });

#elif defined(UI_VIEWS)
  new_row_font_ = ui::ResourceBundle::GetSharedInstance().GetFont(
      ui::ResourceBundle::ITALIC_FONT);

  view_->set_show_hint(true);
  view_->SetPainter(*new TableViewPainter);
  view_->set_controller(this);

  image_list_.reset(new WTL::CImageListManaged);
  image_list_->Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);
  WTL::CBitmap items_bitmap = WTL::AtlLoadBitmapImage(IDB_ITEMS);
  image_list_->Add(items_bitmap, RGB(255, 0, 255));
#endif

  view_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_TABLE_POPUP, point, true);
  });

  selection().multiple_handler = [this] { return GetMultipleSelection(); };
}

TableView::~TableView() {}

#if defined(UI_VIEWS)
bool TableView::OnDrawCell(views::TableView& sender,
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
}
#endif

UiView* TableView::Init(const WindowDefinition& definition) {
  for (WindowItems::const_iterator i = definition.items.begin();
       i != definition.items.end(); i++) {
    const WindowItem& item = *i;

    if (item.name_is("Col")) {
#if defined(UI_VIEWS)
      int i = item.GetInt("ix", 0) - 1;
      if (i < 0 || i >= static_cast<int>(view_->visible_columns().size()))
        continue;

      int width = item.GetInt("width", -1);
      if (width != -1)
        view_->SetVisibleColumnWidth(i, width);
#endif

    } else if (item.name_is("Item")) {
      int ix = item.GetInt("ix", 0) - 1;
      if (ix == -1)
        ix = model_->row_count();
      std::string path = item.GetString("path");
      model_->SetFormula(ix, path);
    }
  }

  return view_->CreateParentIfNecessary();
}

void TableView::Save(WindowDefinition& definition) {
  // Save column widths as default.
  Profile::Columns default_columns;
#if defined(UI_VIEWS)
  for (size_t i = 0; i < view_->visible_columns().size(); ++i) {
    const views::TableView::VisibleColumn& column = view_->visible_columns()[i];
    Profile::Column c = {column.column.id, column.width};
    default_columns.push_back(c);
  }
#endif
  profile_.default_table_columns = std::move(default_columns);

#if defined(UI_VIEWS)
  for (size_t i = 0; i < view_->visible_columns().size(); ++i) {
    WindowItem& item = definition.AddItem("Col");
    item.SetInt("ix", i + 1);
    item.SetInt("width", view_->visible_columns()[i].width);
  }
#endif

  for (int i = 0; i < model_->row_count(); i++) {
    TableRow* row = model_->GetRow(i);
    if (!row)
      continue;
    WindowItem& item = definition.AddItem("Item");
    item.SetInt("ix", i + 1);
    item.SetString("path", row->GetFormula());
  }
}

#if defined(UI_VIEWS)
bool TableView::OnKeyPressed(views::TableView& sender,
                             ui::KeyboardCode key_code) {
  switch (key_code) {
    case ui::VKEY_RETURN:
      if (!view_->editing()) {
        OnDoubleClick();
        return true;
      }
      break;

    case ui::VKEY_DELETE:
      if (!view_->editing()) {
        DeleteSelection();
        return true;
      }
      break;

    case ui::VKEY_UP:
    case ui::VKEY_DOWN:
      if (GetAsyncKeyState(VK_CONTROL) < 0) {
        MoveRow(key_code == ui::VKEY_UP);
        return true;
      }
      break;
  }

  return __super::OnKeyPressed(sender, key_code);
}

bool TableView::OnDoubleClick() {
  if (view_->selection_model().empty())
    return false;

  TableRow* row = model_->GetRow(view_->GetCurrentRow());
  if (!row)
    return false;

  if (row->is_blinking()) {
    row->timed_data().Acknowledge();

  } else {
    const auto& node = row->timed_data().GetNode();
    if (!node)
      return false;

    controller_delegate_.ExecuteDefaultNodeCommand(node);
  }

  return true;
}
#endif

void TableView::AddContainedItem(const scada::NodeId& node_id, unsigned flags) {
  if (!(flags & APPEND))
    model_->Clear();

  auto node = node_service_.GetNode(node_id);
  if (IsInstanceOf(node, id::DataGroupType)) {
    for (auto& child : node.targets(scada::id::HasComponent))
      AddContainedItem(child.node_id(), flags | APPEND);
    return;
  }

  if (node.node_class() != scada::NodeClass::Variable)
    return;

  int ix = model_->FindItem(node_id);
  if (ix != -1) {
#if defined(UI_VIEWS)
    view_->Select(ix, true);
#endif
    return;
  }

  ix = model_->row_count();
  model_->SetFormula(ix, MakeNodeIdFormula(node_id));

#if defined(UI_VIEWS)
  view_->Select(ix, true);
#endif

  controller_delegate_.SetModified(true);
}

void TableView::RemoveContainedItem(const scada::NodeId& node_id) {
  int ix;
  while ((ix = model_->FindItem(node_id)) >= 0)
    model_->DeleteRows(ix, 1);
}

bool TableView::CanClose() const {
  /*if (AtlMessageBox(m_hWnd, _T("Закрытие окна приведет к потере таблицы.
  Продолжить?"), (LPCTSTR)frame->GetTitle(),
  MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2) == IDNO) {
    // cancel close
    return FALSE;
  }*/
  return true;
}

#if defined(UI_VIEWS)
void TableView::OnGetAutocompleteList(views::TableView& sender,
                                      const base::string16& text,
                                      int& start,
                                      std::vector<base::string16>& list) {}
#endif

void TableView::DeleteSelection() {
#if defined(UI_VIEWS)
  typedef ui::ListSelectionModel::SelectedIndices Indices;
  Indices selection = view_->selection_model().selected_indices();
  for (Indices::reverse_iterator i = selection.rbegin(); i != selection.rend();
       ++i)
    model_->DeleteRows(*i, 1);
#endif
}

void TableView::Print(PrintService& print_service) {
  PrintTable(PrintTableContext{print_service, *model_, view_->columns()});
}

#if defined(UI_VIEWS)
void TableView::OnSelectionChanged(views::TableView& sender) {
  if (view_->selection_model().empty())
    selection().Clear();
  else if (view_->selection_model().size() >= 2)
    selection().SelectMultiple();
  else {
    TableRow* row =
        model_->GetRow(view_->selection_model().selected_indices()[0]);
    if (row)
      selection().SelectTimedData(row->timed_data());
    else
      selection().Clear();
  }
}
#endif

NodeIdSet TableView::GetMultipleSelection() {
  NodeIdSet node_ids;

#if defined(UI_QT)
  const auto& ranges = view_->selectionModel()->selection();
  for (auto& range : ranges) {
    for (auto row_index = range.top(); row_index <= range.bottom();
         ++row_index) {
      auto* row = model_->GetRow(row_index);
      const auto& node_id = row->timed_data().GetNode().node_id();
      if (!node_id.is_null())
        node_ids.emplace(node_id);
    }
  }

#elif defined(UI_VIEWS)
  typedef ui::ListSelectionModel::SelectedIndices Indices;
  const Indices& indices = view_->selection_model().selected_indices();
  if (indices.empty())
    return node_ids;

  for (Indices::const_iterator i = indices.begin(); i != indices.end(); ++i) {
    const TableRow* row = model_->GetRow(*i);
    if (!row)
      continue;

    auto node_id = row->timed_data().GetNode().node_id();
    if (!node_id.is_null())
      node_ids.emplace(std::move(node_id));
  }
#endif

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
  switch (command_id) {
    case ID_DELETE:
    case ID_RENAME:
    case ID_MOVE_UP:
    case ID_MOVE_DOWN:
    case ID_SORT_NAME:
    case ID_SORT_CHANNEL:
      return this;
  }

  return __super::GetCommandHandler(command_id);
}

void TableView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_DELETE:
      view_->CloseEditor();
      DeleteSelection();
      return;

    case ID_RENAME:
      view_->OpenEditor(view_->GetCurrentRow());
      return;

    case ID_MOVE_UP:
    case ID_MOVE_DOWN:
      MoveRow(command == ID_MOVE_UP);
      return;

    case ID_SORT_NAME:
    case ID_SORT_CHANNEL:
      model_->Sort(command);
      break;

    default:
      __super::ExecuteCommand(command);
      return;
  }
}

void TableView::MoveRow(bool up) {
  int row = view_->GetCurrentRow();
  if (row == -1)
    return;

  auto row2 = model_->MoveRow(row, up);
  if (row2 != -1)
    view_->SelectRow(row2, true);
}

#if defined(UI_QT)
void TableView::OnSelectionChanged() {
  const auto& ranges = view_->selectionModel()->selection();
  if (ranges.empty()) {
    selection().Clear();
  } else if (ranges.size() == 1 &&
             ranges.front().top() == ranges.front().bottom()) {
    auto* row = model_->GetRow(ranges.front().top());
    if (row)
      selection().SelectTimedData(row->timed_data());
    else
      selection().Clear();
  } else {
    selection().SelectMultiple();
  }
}
#endif
