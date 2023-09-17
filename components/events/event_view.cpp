#include "components/events/event_view.h"

#include "aui/models/table_column.h"
#include "aui/table.h"
#include "base/excel.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "client_utils.h"
#include "common/event_fetcher.h"
#include "common_resources.h"
#include "components/events/current_event_model.h"
#include "components/events/event_table_model.h"
#include "components/prompt/prompt_dialog.h"
#include "components/time_range/time_range_dialog.h"
#include "contents_observer.h"
#include "controller_delegate.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "selection_model.h"
#include "services/dialog_service.h"
#include "services/profile.h"
#include "window_definition_util.h"

namespace {

struct EventTableModelHolder {
  EventTableModelHolder(const ControllerContext& context, bool is_panel)
      : current_event_model{context.node_event_provider_},
        event_table_model{{context.executor_, context.node_service_,
                           current_event_model, context.local_events_,
                           context.history_service_, is_panel}} {}

  CurrentEventModel current_event_model;
  EventTableModel event_table_model;
};

std::shared_ptr<EventTableModel> CreateEventTableModel(
    const ControllerContext& context,
    bool is_panel) {
  auto holder = std::make_shared<EventTableModelHolder>(context, is_panel);
  return {holder, &holder->event_table_model};
}

}  // namespace

// EventView

EventView::EventView(const ControllerContext& context, bool is_panel)
    : ControllerContext{context},
      is_panel_{is_panel},
      model_{CreateEventTableModel(context, is_panel)} {
  const aui::TableColumn kEventViewColumns[] = {
      {EventColumnTime, u"Время", 150, aui::TableColumn::LEFT,
       aui::TableColumn::DataType::DateTime},
      {EventColumnItem, u"Объект", 170, aui::TableColumn::LEFT},
      {EventColumnSeverity, u"Важность", 45, aui::TableColumn::RIGHT},
      {EventColumnValue, u"Значение", 100, aui::TableColumn::RIGHT},
      {EventColumnMessage, u"Сообщение", 300, aui::TableColumn::LEFT},
      {EventColumnUser, u"Инициатор", 100, aui::TableColumn::LEFT},
      {EventColumnAckUser, u"Квитировал", 100, aui::TableColumn::LEFT},
      {EventColumnAckTime, u"Время квитирования", 150, aui::TableColumn::LEFT,
       aui::TableColumn::DataType::DateTime},
  };

  size_t count = std::size(kEventViewColumns);
  if (is_panel)
    count -= 2;

  table_ = new aui::Table{model_,
                          std::vector<aui::TableColumn>(
                              kEventViewColumns, kEventViewColumns + count),
                          true};

#if defined(UI_QT)
  table_->sortByColumn(0, Qt::DescendingOrder);
#endif

  table_->SetContextMenuHandler([this](const aui::Point& point) {
    controller_delegate_.ShowPopupMenu(IDR_EVENT_POPUP, point, true);
  });

  table_->SetSelectionChangeHandler([this] { OnSelectionChanged(); });

  table_->SetDoubleClickHandler([this] { AcknowledgeSelection(); });

  table_->SetKeyPressHandler(
      [this](aui::KeyCode key_code) { return OnKeyPressed(key_code); });

  selection_.multiple_handler = [this] { return GetSelectedNodeIds(); };

  command_registry_.AddCommand(
      Command{ID_ACKNOWLEDGE_CURRENT}
          .set_execute_handler([this] { AcknowledgeSelection(); })
          .set_enabled_handler([this] { return CanAcknowledgeSelection(); }));

  command_registry_.AddCommand(
      Command{ID_SEVERITY_ALL}
          .set_execute_handler([this] {
            model_->SetSeverityMin(0);
            controller_delegate_.SetTitle(MakeTitle());
          })
          .set_checked_handler([this] { return model_->severity_min() == 0; }));

  command_registry_.AddCommand(
      Command{ID_SEVERITY_CUSTOM}
          .set_execute_handler([this] { SelectSeverity(); })
          .set_checked_handler([this] { return model_->severity_min() != 0; }));
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
    selection_.Clear();
  else if (rows.size() >= 2)
    selection_.SelectMultiple();
  else {
    const scada::Event& event = model_->event_at(rows.front());
    selection_.SelectNode(node_service_.GetNode(event.node_id));
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

bool EventView::OnKeyPressed(aui::KeyCode key_code) {
  switch (key_code) {
    case aui::KeyCode::Escape:
      model_->CancelRequest();
      return true;

    default:
      return false;
  }
}

bool EventView::IsWorking() const {
  return model_->IsWorking();
}

std::u16string EventView::MakeTitle() const {
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
    dialog_service_.RunMessageBox(u"Нет данных для экспорта.", u"Экспорт",
                                  MessageBoxMode::Info);
    return;
  }

  try {
    ExcelSheetModel sheet{rows + 1, EventColumnCount};

    const auto& columns = table_->columns();

    for (size_t i = 0; i < columns.size(); ++i)
      sheet.SetData(1, i + 1, base::AsWString(columns[i].title));

    for (int row = 0; row < model_->GetRowCount(); ++row) {
      for (size_t col = 0; col < columns.size(); ++col) {
        auto text = model_->GetCellText(row, columns[col].id);
        sheet.SetData(2 + row, col + 1, base::AsWString(text));
      }
    }

    Excel excel;
    excel.NewWorkbook();
    excel.NewSheet(sheet);
    excel.SetVisible();

  } catch (HRESULT /*err*/) {
    dialog_service_.RunMessageBox(u"Ошибка при экспорте.", u"Экспорт",
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
  return command_registry_.GetCommandHandler(command_id);
}

void EventView::SetTimeRange(const TimeRange& time_range) {
  model_->SetTimeRange(time_range);
  controller_delegate_.SetTitle(MakeTitle());
}

promise<> EventView::SelectSeverity() {
  unsigned initial_severity = model_->current_events()
                                  ? node_event_provider_.severity_min()
                                  : model_->severity_min();
  const char16_t prompt[] = u"Минимальный порог важности (0 - все события):";
  return RunPromptDialog(dialog_service_, prompt, u"Фильтр",
                         base::NumberToString16(initial_severity))
      .then([this](const std::u16string& input) {
        unsigned severity = 0;
        if (!base::StringToUint(input, &severity) ||
            severity > scada::kSeverityMax) {
          auto message =
              base::StringPrintf(u"Введите число от %d до %d.",
                                 scada::kSeverityMin, scada::kSeverityMax);
          return ToRejectedPromise<unsigned>(dialog_service_.RunMessageBox(
              message, u"Фильтр", MessageBoxMode::Error));
        }

        return make_resolved_promise(severity);
      })
      .then([this](unsigned severity) {
        if (model_->current_events()) {
          node_event_provider_.SetSeverityMin(severity);
        } else {
          model_->SetSeverityMin(severity);
          controller_delegate_.SetTitle(MakeTitle());
        }
      });
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
