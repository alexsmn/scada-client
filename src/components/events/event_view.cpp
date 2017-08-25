#include "components/events/event_view.h"

#include "base/excel.h"
#include "common_resources.h"
#include "commands/prompt_dialog.h"
#include "controller_factory.h"
#include "selection_model.h"
#include "commands/time_range_dialog.h"
#include "components/events/event_table_model.h"
#include "components/main/main_window.h"
#include "common/event_manager.h"
#include "client_utils.h"
#include "controls/table.h"
#include "contents_observer.h"

#if defined(UI_QT)
#include <qheaderview.h>
#elif defined(UI_VIEWS)
#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atlctrls.h>
#endif

static const char* FormatTimeRange(unsigned mode) {
  switch (mode) {
    case ID_TIME_RANGE_DAY:
      return "Day";
    case ID_TIME_RANGE_WEEK:
      return "Week";
    case ID_TIME_RANGE_MONTH:
      return "Month";
    default:
      return "Current";
  }
}

class EventPanel : public EventView {
 public:
  explicit EventPanel(const ControllerContext& context) : EventView(context, true) {}
};

class EventJournal : public EventView {
 public:
  explicit EventJournal(const ControllerContext& context) : EventView(context, false) {}
};

REGISTER_CONTROLLER(EventPanel, ID_EVENT_VIEW);
REGISTER_CONTROLLER(EventJournal, ID_EVENT_JOURNAL_VIEW);

// EventView

EventView::EventView(const ControllerContext& context, bool is_panel)
    : Controller(context),
      is_panel_(is_panel),
#if defined(UI_VIEWS)
      severities_image_list_(new WTL::CImageListManaged),
#endif
      model_(std::make_unique<EventTableModel>(EventTableModelContext{
          context.node_service_,
          context.event_manager_,
          context.local_events_,
          context.history_service_,
      })) {
  const ui::TableColumn kEventViewColumns[] = {
      ui::TableColumn(EventColumnTime, L"Время", 150, ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnItem, L"Объект", 170, ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnSeverity, L"Важность", 45, ui::TableColumn::RIGHT),
      ui::TableColumn(EventColumnValue, L"Значение", 100, ui::TableColumn::RIGHT),
      ui::TableColumn(EventColumnMessage, L"Сообщение", 300, ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnSource, L"Источник", 100, ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnAckUser, L"Квитировал", 100, ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnAckTime, L"Время квитирования", 150, ui::TableColumn::LEFT)
  };

  size_t count = _countof(kEventViewColumns);
  if (is_panel)
    count -= 2;

  table_.reset(new Table(*model_, std::vector<ui::TableColumn>(kEventViewColumns, kEventViewColumns + count)));

#if defined(UI_QT)
  table_->horizontalHeader()->setHighlightSections(false);
  table_->verticalHeader()->setDefaultSectionSize(19);
  table_->setShowGrid(false);
  table_->resizeColumnsToContents();

#elif defined(UI_VIEWS)
  severities_image_list_->Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);
  WTL::CBitmap severities_bitmap = WTL::AtlLoadBitmapImage(IDB_EVENT_SEVERITIES);
  severities_image_list_->Add(severities_bitmap, RGB(0, 255, 0));

  //table_->set_show_grid(true);

  table_->SetColumns(count, kEventViewColumns);
  table_->set_controller(this);
#endif

  selection().multiple_handler_ = [this] { return GetSelectedNodeIds(); };
}

EventView::~EventView() {
}

NodeIdSet EventView::GetContainedItems() const {
  return model_->filter_items();
}

void EventView::AcknowledgeSelection() {
#if defined(UI_VIEWS)
  typedef ui::ListSelectionModel::SelectedIndices Indices;
  Indices indices = table_->selection_model().selected_indices();
  for (Indices::reverse_iterator i = indices.rbegin(); i != indices.rend(); ++i)
    model_->AcknowledgeRow(*i);
#endif
}

#if defined(UI_VIEWS)
void EventView::OnSelectionChanged(views::TableView& sender) {
  if (table_->selection_model().empty())
    selection().Clear();
  else  if (table_->selection_model().selected_indices().size() >= 2)
    selection().SelectMultiple();
  else {
    auto& indices = table_->selection_model().selected_indices();
    const scada::Event& event = model_->event_at(indices[0]);
    selection().SelectNodeId(event.node_id);
  }
}
#endif

bool EventView::CanAcknowledgeSelection() const {
#if defined(UI_VIEWS)
  typedef ui::ListSelectionModel::SelectedIndices Indices;
  const Indices& indices = table_->selection_model().selected_indices();
  for (Indices::const_iterator i = indices.begin(); i != indices.end(); ++i) {
    const scada::Event& event = model_->event_at(*i);
    if (!event.acked)
      return true;
  }
#endif
  return false;
}

#if defined(UI_VIEWS)
bool EventView::OnDoubleClick() {
  AcknowledgeSelection();
  return true;
}
#endif

UiView* EventView::Init(const WindowDefinition& definition) {
  EventTableModel::ItemIds filter_items;

  WORD mode = is_panel_ ? ID_CURRENT_EVENTS : ID_TIME_RANGE_DAY;
  TimeRange range;
  
  for (WindowItems::const_iterator i = definition.items.begin();
                                   i != definition.items.end(); ++i) {
    const WindowItem& window_item = *i;
    if (window_item.name_is("Item")) {
      std::string path = window_item.GetString("path");
      auto item_id = scada::NodeId::FromString(path);
      if (!item_id.is_null())
        filter_items.insert(item_id);

    } else if (window_item.name_is("Column")) {
#if defined(UI_VIEWS)
      int index = window_item.GetInt("index", -1);
      int width = window_item.GetInt("width", 0);
      if (index >= 0 && index < static_cast<int>(table_->visible_columns().size()) && width > 0)
        table_->SetVisibleColumnWidth(index, width);
#endif

    } else if (window_item.name_is("Window")) {
      std::string smode = window_item.GetString("mode");
      if (_stricmp(smode.c_str(), "Day") == 0)
        mode = ID_TIME_RANGE_DAY;
      else if (_stricmp(smode.c_str(), "Week") == 0)
        mode = ID_TIME_RANGE_WEEK;
      else if (_stricmp(smode.c_str(), "Month") == 0)
        mode = ID_TIME_RANGE_MONTH;
      else if (_stricmp(smode.c_str(), "Current") == 0)
        mode = ID_CURRENT_EVENTS;
      else if (_stricmp(smode.c_str(), "GridRange") == 0) {
        mode = ID_TIME_RANGE_CUSTOM;

      }
    }
  }

  model_->Init(mode, range, filter_items);

  if (!is_panel_)
    controller_delegate_.SetTitle(MakeTitle());

#if defined(UI_QT)
  return table_.get();

#elif defined(UI_VIEWS)
  return &table_->CreateParentIfNecessary();
#endif
}

#if defined(UI_VIEWS)
void EventView::ShowContextMenu(gfx::Point point) {
  controller_delegate_.ShowPopupMenu(IDR_EVENT_POPUP, point, true);
}
#endif

#if defined(UI_VIEWS)
bool EventView::OnKeyPressed(views::TableView& sender, ui::KeyboardCode key_code) {
  switch (key_code) {
    case VK_ESCAPE:
      model_->CancelRequest();
      return true;

    default:
      return false;
  }
}
#endif

bool EventView::IsWorking() const {
  return model_->IsWorking();
}

base::string16 EventView::MakeTitle() const {
  return model_->MakeTitle();
}

void EventView::Save(WindowDefinition& definition) {
  const char* mode_string = FormatTimeRange(model_->mode());
  definition.AddItem("Window").SetString("mode", mode_string);

#if defined(UI_VIEWS)
  for (size_t i = 0; i < table_->visible_columns().size(); ++i) {
    WindowItem& window_item = definition.AddItem("Column");
    window_item.SetInt("index", i);
    window_item.SetInt("width", table_->visible_columns()[i].width);
  }
#endif

  for (NodeIdSet::const_iterator i = model_->filter_items().begin();
                                 i != model_->filter_items().end(); ++i) {
    const scada::NodeId& item = *i;
    WindowItem& window_item = definition.AddItem("Item");
    window_item.SetString("path", item.ToString());
  }
}

void EventView::ExportToExcel() {
  int rows = model_->GetRowCount();
  if (!rows) {
    ShowMessageBox(dialog_service_, L"Нет данных для экспорта.", L"Экспорт", MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  try {
    ExcelSheetModel sheet;

    sheet.SetDataSize(rows + 1, EventColumnCount);

#if defined(UI_VIEWS)
    const views::TableView::TableColumns& columns = table_->columns();

    for (size_t i = 0; i < columns.size(); ++i)
      sheet.SetData(1, i + 1, base::win::ScopedVariant(columns[i].title.c_str()));

    for (int row = 0; row < model_->GetRowCount(); ++row) {
      for (size_t col = 0; col < columns.size(); ++col) {
        base::string16 text = model_->GetCellText(row, columns[col].id);
        sheet.SetData(2 + row, col + 1, base::win::ScopedVariant(text.c_str()));
      }
    }
#endif

    Excel excel;
    excel.NewWorkbook();
    excel.NewSheet(sheet);
    excel.SetVisible();

  } catch (HRESULT /*err*/) {
    ShowMessageBox(dialog_service_, L"Ошибка при экспорте.", L"Экспорт", MB_OK | MB_ICONSTOP);
  }
}

void EventView::AddContainedItem(const scada::NodeId& node_id, unsigned flags) {
  if (is_panel_)
    return;

  if (model_->AddFilteredItem(node_id) && contents_observer())
    contents_observer()->OnContainedItemChanged(node_id, true);
}

void EventView::RemoveContainedItem(const scada::NodeId& node_id) {
  if (is_panel_)
    return;

  if (model_->RemoveFilteredItem(node_id) && contents_observer())
    contents_observer()->OnContainedItemChanged(node_id, false);
}

CommandHandler* EventView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_ACKNOWLEDGE_CURRENT:
    case ID_EXPORT:
    case ID_SEVERITY_CUSTOM:
    case ID_SEVERITY_ALL:
      return this;
      
    case ID_CURRENT_EVENTS:
    case ID_TIME_RANGE_DAY:
    case ID_TIME_RANGE_WEEK:
    case ID_TIME_RANGE_MONTH:
    case ID_TIME_RANGE_CUSTOM:
      return !is_panel_ ? this : NULL;
      
    default:
      return __super::GetCommandHandler(command_id);
  }
}

bool EventView::IsCommandEnabled(unsigned command_id) const {
  switch (command_id) {
    case ID_ACKNOWLEDGE_CURRENT:
      return CanAcknowledgeSelection();
      
    default:
      return __super::IsCommandEnabled(command_id);
  }
}

bool EventView::IsCommandChecked(unsigned command_id) const {
  switch (command_id) {
    case ID_CURRENT_EVENTS:
    case ID_TIME_RANGE_DAY:
    case ID_TIME_RANGE_WEEK:
    case ID_TIME_RANGE_MONTH:
    case ID_TIME_RANGE_CUSTOM:
      return command_id == model_->mode();
    case ID_SEVERITY_CUSTOM:
      return model_->severity_min() != 0;
    case ID_SEVERITY_ALL:
      return model_->severity_min() == 0;
    default:
      return __super::IsCommandChecked(command_id);
  }
}

void EventView::ExecuteCommand(unsigned command) {
  switch (command) {
    case ID_ACKNOWLEDGE_CURRENT:
      AcknowledgeSelection();
      break;

    case ID_SEVERITY_ALL:
      model_->SetSeverityMin(0);
      controller_delegate_.SetTitle(MakeTitle());
      break;

    case ID_SEVERITY_CUSTOM:
      SelectSeverity();
      break;

    case ID_CURRENT_EVENTS:
    case ID_TIME_RANGE_DAY:
    case ID_TIME_RANGE_WEEK:
    case ID_TIME_RANGE_MONTH:
      model_->SetMode(command);
      controller_delegate_.SetTitle(MakeTitle());
      break;

    case ID_TIME_RANGE_CUSTOM:
      SelectTimeRange();
      break;

    case ID_EXPORT:
      ExportToExcel();
      break;

    default:
      __super::ExecuteCommand(command);
      break;
  }
}

void EventView::SelectTimeRange() {
  DCHECK(!is_panel_);

  auto range = model_->time_range();
  if (!ShowTimeRangeDialog(dialog_service_, range))
    return;

  model_->set_time_range(range);
  model_->SetMode(ID_TIME_RANGE_CUSTOM);

  controller_delegate_.SetTitle(MakeTitle());
}

void EventView::SelectSeverity() {
  unsigned severity = (model_->mode() == ID_CURRENT_EVENTS) ?
      event_manager_.severity_min() : model_->severity_min();
  const base::char16 prompt[] =
      L"Минимальный порог важности (0 - все события):";
  base::string16 value = WideFormat(severity);
  for (;;) {
    if (!RunPromptDialog(dialog_service_, prompt, L"Фильтр", value))
      return;
    if (!Parse(value, severity) || severity > scada::kSeverityMax) {
      base::string16 message = base::StringPrintf(L"Введите число от %d до %d.",
          scada::kSeverityMin, scada::kSeverityMax);
      MessageBox(GetActiveWindow(), message.c_str(), L"Фильтр", MB_ICONSTOP);
      continue;
    }
    break;
  }

  if (model_->mode() == ID_CURRENT_EVENTS) {
    event_manager_.SetSeverityMin(severity);
  } else {
    model_->SetSeverityMin(severity);
    controller_delegate_.SetTitle(MakeTitle());
  }
}

NodeIdSet EventView::GetSelectedNodeIds() const {
  NodeIdSet node_ids;
#if defined(UI_VIEWS)
  typedef ui::ListSelectionModel::SelectedIndices Indices;
  const Indices& indices = table_->selection_model().selected_indices();
  for (Indices::const_iterator i = indices.begin(); i != indices.end(); ++i) {
    const scada::Event& event = model_->event_at(*i);
    if (!event.node_id.is_null())
      node_ids.insert(event.node_id);
  }
#endif
  return node_ids;
}
