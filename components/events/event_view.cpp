#include "components/events/event_view.h"

#include "base/excel.h"
#include "client_utils.h"
#include "commands/prompt_dialog.h"
#include "commands/time_range_dialog.h"
#include "common/event_manager.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "common_resources.h"
#include "components/events/event_table_model.h"
#include "contents_observer.h"
#include "controller_factory.h"
#include "controls/table.h"
#include "selection_model.h"
#include "services/dialog_service.h"

#if defined(UI_QT)
#include <QHeaderView>
#elif defined(UI_VIEWS)
#include <atlbase.h>

#include <atlapp.h>
#include <atlctrls.h>
#endif

class EventPanel : public EventView {
 public:
  explicit EventPanel(const ControllerContext& context)
      : EventView(context, true) {}
};

class EventJournal : public EventView {
 public:
  explicit EventJournal(const ControllerContext& context)
      : EventView(context, false) {}
};

REGISTER_CONTROLLER(EventPanel, ID_EVENT_VIEW);
REGISTER_CONTROLLER(EventJournal, ID_EVENT_JOURNAL_VIEW);

// EventView

EventView::EventView(const ControllerContext& context, bool is_panel)
    : Controller{context},
      is_panel_{is_panel},
#if defined(UI_VIEWS)
      severities_image_list_{std::make_unique<WTL::CImageList>()},
#endif
      model_{std::make_unique<EventTableModel>(EventTableModelContext{
          context.node_service_, context.event_manager_, context.local_events_,
          context.history_service_, is_panel_})} {
  const ui::TableColumn kEventViewColumns[] = {
      ui::TableColumn(EventColumnTime, L"Время", 150, ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnItem, L"Объект", 170, ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnSeverity, L"Важность", 45,
                      ui::TableColumn::RIGHT),
      ui::TableColumn(EventColumnValue, L"Значение", 100,
                      ui::TableColumn::RIGHT),
      ui::TableColumn(EventColumnMessage, L"Сообщение", 300,
                      ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnUser, L"Инициатор", 100,
                      ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnAckUser, L"Квитировал", 100,
                      ui::TableColumn::LEFT),
      ui::TableColumn(EventColumnAckTime, L"Время квитирования", 150,
                      ui::TableColumn::LEFT)};

  size_t count = std::size(kEventViewColumns);
  if (is_panel)
    count -= 2;

  table_.reset(
      new Table(*model_, std::vector<ui::TableColumn>(
                             kEventViewColumns, kEventViewColumns + count)));

#if defined(UI_QT)
  table_->horizontalHeader()->setHighlightSections(false);
  table_->verticalHeader()->setDefaultSectionSize(19);
  table_->setShowGrid(false);
  table_->resizeColumnsToContents();

#elif defined(UI_VIEWS)
  severities_image_list_->Create(16, 16, ILC_MASK | ILC_COLOR32, 0, 0);
  WTL::CBitmap severities_bitmap =
      WTL::AtlLoadBitmapImage(IDB_EVENT_SEVERITIES);
  severities_image_list_->Add(severities_bitmap, RGB(0, 255, 0));

  table_->SetColumns(count, kEventViewColumns);
  table_->set_controller(this);
#endif

  table_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_EVENT_POPUP, point, true);
  });

  selection().multiple_handler = [this] { return GetSelectedNodeIds(); };
}

EventView::~EventView() {
#if defined(UI_VIEWS)
  severities_image_list_->Destroy();
#endif
}

NodeIdSet EventView::GetContainedItems() const {
  return model_->filter_items();
}

void EventView::AcknowledgeSelection() {
  auto rows = table_->GetSelectedRows();
  for (auto i = rows.rbegin(); i != rows.rend(); ++i)
    model_->AcknowledgeRow(*i);
}

#if defined(UI_VIEWS)
void EventView::OnSelectionChanged(views::TableView& sender) {
  if (table_->selection_model().empty())
    selection().Clear();
  else if (table_->selection_model().selected_indices().size() >= 2)
    selection().SelectMultiple();
  else {
    auto& indices = table_->selection_model().selected_indices();
    const scada::Event& event = model_->event_at(indices[0]);
    selection().SelectNode(node_service_.GetNode(event.node_id));
  }
}
#endif

bool EventView::CanAcknowledgeSelection() const {
  auto rows = table_->GetSelectedRows();
  for (auto i = rows.begin(); i != rows.end(); ++i) {
    const scada::Event& event = model_->event_at(*i);
    if (!event.acked)
      return true;
  }
  return false;
}

#if defined(UI_VIEWS)
bool EventView::OnDoubleClick() {
  AcknowledgeSelection();
  return true;
}
#endif

UiView* EventView::Init(const WindowDefinition& definition) {
  model_->LockUpdate();
  model_->Update();

  WORD mode = is_panel_ ? ID_CURRENT_EVENTS : ID_TIME_RANGE_DAY;
  TimeRange range;

  for (auto& window_item : definition.items) {
    if (window_item.name_is("Item")) {
      std::string path = window_item.GetString("path");
      auto item_id = NodeIdFromScadaString(path);
      if (!item_id.is_null())
        model_->AddFilteredItem(item_id);

    } else if (window_item.name_is("Column")) {
#if defined(UI_VIEWS)
      int index = window_item.GetInt("index", -1);
      int width = window_item.GetInt("width", 0);
      if (index >= 0 &&
          index < static_cast<int>(table_->visible_columns().size()) &&
          width > 0)
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
      else if (_stricmp(smode.c_str(), "Custom") == 0) {
        mode = ID_TIME_RANGE_CUSTOM;
      }
    }
  }

  model_->SetTimeRange(range);
  model_->UnlockUpdate();

  if (!is_panel_)
    controller_delegate_.SetTitle(MakeTitle());

#if defined(UI_QT)
  return table_.get();

#elif defined(UI_VIEWS)
  return table_->CreateParentIfNecessary();
#endif
}

#if defined(UI_VIEWS)
bool EventView::OnKeyPressed(views::TableView& sender,
                             ui::KeyboardCode key_code) {
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
  const char* mode_string = FormatTimeRange(model_->time_range().command_id);
  definition.AddItem("Window").SetString("mode", mode_string);

#if defined(UI_VIEWS)
  for (size_t i = 0; i < table_->visible_columns().size(); ++i) {
    WindowItem& window_item = definition.AddItem("Column");
    window_item.SetInt("index", i);
    window_item.SetInt("width", table_->visible_columns()[i].width);
  }
#endif

  for (auto& item : model_->filter_items()) {
    WindowItem& window_item = definition.AddItem("Item");
    window_item.SetString("path", NodeIdToScadaString(item));
  }
}

void EventView::ExportToExcel() {
  int rows = model_->GetRowCount();
  if (!rows) {
    dialog_service_.RunMessageBox(L"Нет данных для экспорта.", L"Экспорт",
                                  MessageBoxMode::Info);
    return;
  }

  try {
    ExcelSheetModel sheet;

    sheet.SetDataSize(rows + 1, EventColumnCount);

#if defined(UI_VIEWS)
    const views::TableView::TableColumns& columns = table_->columns();

    for (size_t i = 0; i < columns.size(); ++i)
      sheet.SetData(1, i + 1,
                    base::win::ScopedVariant(columns[i].title.c_str()));

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
    dialog_service_.RunMessageBox(L"Ошибка при экспорте.", L"Экспорт",
                                  MessageBoxMode::Error);
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

TimeRange EventView::GetTimeRange() const {
  return model_->time_range();
}

CommandHandler* EventView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_ACKNOWLEDGE_CURRENT:
    case ID_EXPORT:
    case ID_SEVERITY_CUSTOM:
    case ID_SEVERITY_ALL:
      return this;

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

    case ID_EXPORT:
      ExportToExcel();
      break;

    default:
      __super::ExecuteCommand(command);
      break;
  }
}

void EventView::SetTimeRange(const TimeRange& time_range) {
  model_->SetTimeRange(time_range);
  controller_delegate_.SetTitle(MakeTitle());
}

void EventView::SelectSeverity() {
  unsigned severity = model_->current_events() ? event_manager_.severity_min()
                                               : model_->severity_min();
  const base::char16 prompt[] =
      L"Минимальный порог важности (0 - все события):";
  base::string16 value = WideFormat(severity);
  for (;;) {
    if (!RunPromptDialog(dialog_service_, prompt, L"Фильтр", value))
      return;
    if (!Parse(value, severity) || severity > scada::kSeverityMax) {
      base::string16 message =
          base::StringPrintf(L"Введите число от %d до %d.", scada::kSeverityMin,
                             scada::kSeverityMax);
      MessageBox(GetActiveWindow(), message.c_str(), L"Фильтр", MB_ICONSTOP);
      continue;
    }
    break;
  }

  if (model_->current_events()) {
    event_manager_.SetSeverityMin(severity);
  } else {
    model_->SetSeverityMin(severity);
    controller_delegate_.SetTitle(MakeTitle());
  }
}

NodeIdSet EventView::GetSelectedNodeIds() const {
  NodeIdSet node_ids;
  for (auto row : table_->GetSelectedRows()) {
    const scada::Event& event = model_->event_at(row);
    if (!event.node_id.is_null())
      node_ids.insert(event.node_id);
  }
  return node_ids;
}

TimeModel* EventView::GetTimeModel() {
  return model_->current_events() ? nullptr : this;
}
