#include "components/events/event_view.h"

#include "base/excel.h"
#include "base/strings/string_util.h"
#include "client_utils.h"
#include "common/event_manager.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "common_resources.h"
#include "components/events/event_table_model.h"
#include "components/prompt/prompt_dialog.h"
#include "components/time_range/time_range_dialog.h"
#include "contents_observer.h"
#include "controller_factory.h"
#include "controls/table.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "services/profile.h"
#include "window_definition_util.h"

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

const WindowInfo kEventWindowInfo = {
    ID_EVENT_VIEW, "Event", L"События", WIN_SING | WIN_DOCKB | WIN_CAN_PRINT,
    800,           200,     0};

const WindowInfo kEventJournalWindowInfo = {ID_EVENT_JOURNAL_VIEW,
                                            "EventJournal",
                                            L"Журнал событий",
                                            WIN_INS | WIN_CAN_PRINT,
                                            0,
                                            0,
                                            0};

REGISTER_CONTROLLER(EventPanel, kEventWindowInfo);
REGISTER_CONTROLLER(EventJournal, kEventJournalWindowInfo);

// EventView

EventView::EventView(const ControllerContext& context, bool is_panel)
    : Controller{context},
      is_panel_{is_panel},
      model_{std::make_unique<EventTableModel>(EventTableModelContext{
          context.node_service_, context.event_manager_, context.local_events_,
          context.history_service_, is_panel_})} {
  const ui::TableColumn kEventViewColumns[] = {
      {EventColumnTime, L"Время", 150, ui::TableColumn::LEFT,
       ui::TableColumn::DataType::DateTime},
      {EventColumnItem, L"Объект", 170, ui::TableColumn::LEFT},
      {EventColumnSeverity, L"Важность", 45, ui::TableColumn::RIGHT},
      {EventColumnValue, L"Значение", 100, ui::TableColumn::RIGHT},
      {EventColumnMessage, L"Сообщение", 300, ui::TableColumn::LEFT},
      {EventColumnUser, L"Инициатор", 100, ui::TableColumn::LEFT},
      {EventColumnAckUser, L"Квитировал", 100, ui::TableColumn::LEFT},
      {EventColumnAckTime, L"Время квитирования", 150, ui::TableColumn::LEFT,
       ui::TableColumn::DataType::DateTime},
  };

  size_t count = std::size(kEventViewColumns);
  if (is_panel)
    count -= 2;

  table_.reset(new Table(*model_,
                         std::vector<ui::TableColumn>(
                             kEventViewColumns, kEventViewColumns + count),
                         true));

#if defined(UI_QT)
  table_->sortByColumn(0, Qt::DescendingOrder);
#endif

  table_->SetContextMenuHandler([this](const UiPoint& point) {
    controller_delegate_.ShowPopupMenu(IDR_EVENT_POPUP, point, true);
  });

  table_->SetSelectionChangeHandler([this] { OnSelectionChanged(); });

  table_->SetDoubleClickHandler([this] { AcknowledgeSelection(); });

  table_->SetKeyPressHandler(
      [this](KeyCode key_code) { return OnKeyPressed(key_code); });

  selection().multiple_handler = [this] { return GetSelectedNodeIds(); };
}

EventView::~EventView() {}

NodeIdSet EventView::GetContainedItems() const {
  return model_->filter_items();
}

void EventView::AcknowledgeSelection() {
  auto rows = table_->GetSelectedRows();
  for (auto i = rows.rbegin(); i != rows.rend(); ++i)
    model_->AcknowledgeRow(*i);
}

void EventView::OnSelectionChanged() {
  auto rows = table_->GetSelectedRows();
  if (rows.empty())
    selection().Clear();
  else if (rows.size() >= 2)
    selection().SelectMultiple();
  else {
    const scada::Event& event = model_->event_at(rows.front());
    selection().SelectNode(node_service_.GetNode(event.node_id));
  }
}

bool EventView::CanAcknowledgeSelection() const {
  auto rows = table_->GetSelectedRows();
  for (auto i = rows.begin(); i != rows.end(); ++i) {
    const scada::Event& event = model_->event_at(*i);
    if (!event.acked)
      return true;
  }
  return false;
}

UiView* EventView::Init(const WindowDefinition& definition) {
  model_->LockUpdate();
  model_->Update();

  for (auto& window_item : definition.items) {
    if (window_item.name_is("Item")) {
      auto path = window_item.GetString("path");
      auto item_id = NodeIdFromScadaString(path);
      if (!item_id.is_null())
        model_->AddFilteredItem(item_id);
    }
  }

  if (!is_panel_) {
    if (auto time_range = RestoreTimeRange(definition))
      model_->SetTimeRange(*time_range);
  }

  if (auto* state = definition.FindItem("State"))
    table_->RestoreState(state->attributes);
  else if (!is_panel_)
    table_->RestoreState(profile_.event_journal.default_state);

  model_->UnlockUpdate();

  if (!is_panel_)
    controller_delegate_.SetTitle(MakeTitle());

  return table_->CreateParentIfNecessary();
}

bool EventView::OnKeyPressed(KeyCode key_code) {
  switch (key_code) {
    case KeyCode::Escape:
      model_->CancelRequest();
      return true;

    default:
      return false;
  }
}

bool EventView::IsWorking() const {
  return model_->IsWorking();
}

base::string16 EventView::MakeTitle() const {
  return model_->MakeTitle();
}

void EventView::Save(WindowDefinition& definition) {
  definition.AddItem("State").attributes = table_->SaveState();

  if (!is_panel_)
    SaveTimeRange(definition, model_->time_range());

  for (auto& item : model_->filter_items()) {
    WindowItem& window_item = definition.AddItem("Item");
    window_item.SetString("path", NodeIdToScadaString(item));
  }

  if (!is_panel_)
    profile_.event_journal.default_state = table_->SaveState();
}

void EventView::ExportToExcel() {
  int rows = model_->GetRowCount();
  if (!rows) {
    dialog_service_.RunMessageBox(L"Нет данных для экспорта.", L"Экспорт",
                                  MessageBoxMode::Info);
    return;
  }

  try {
    ExcelSheetModel sheet{rows + 1, EventColumnCount};

    const auto& columns = table_->columns();

    for (size_t i = 0; i < columns.size(); ++i)
      sheet.SetData(1, i + 1, columns[i].title);

    for (int row = 0; row < model_->GetRowCount(); ++row) {
      for (size_t col = 0; col < columns.size(); ++col) {
        base::string16 text = model_->GetCellText(row, columns[col].id);
        sheet.SetData(2 + row, col + 1, text);
      }
    }

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

  if (model_->AddFilteredItem(node_id))
    NotifyContainedItemChanged(node_id, true);
}

void EventView::RemoveContainedItem(const scada::NodeId& node_id) {
  if (is_panel_)
    return;

  if (model_->RemoveFilteredItem(node_id))
    NotifyContainedItemChanged(node_id, false);
}

TimeRange EventView::GetTimeRange() const {
  return model_->time_range();
}

CommandHandler* EventView::GetCommandHandler(unsigned command_id) {
  switch (command_id) {
    case ID_ACKNOWLEDGE_CURRENT:
    case ID_SEVERITY_CUSTOM:
    case ID_SEVERITY_ALL:
      return this;

    default:
      return Controller::GetCommandHandler(command_id);
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

ExportModel::ExportData EventView::GetExportData() {
  return TableExportData{*model_, table_->columns()};
}
