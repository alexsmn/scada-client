#include "events/event_view.h"

#include "aui/dialog_service.h"
#include "aui/models/table_column.h"
#include "aui/prompt_dialog.h"
#include "aui/resource_error.h"
#include "aui/show_message_box.h"
#include "aui/table.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/excel.h"
#include "base/format.h"
#include "base/u16format.h"
#include "base/utf_convert.h"
#include "controller/controller_delegate.h"
#include "controller/selection_model.h"
#include "events/current_event_model.h"
#include "events/event_table_model.h"
#include "events/historical_event_model.h"
#include "events/local_event_model.h"
#include "model/node_id_util.h"
#include "modules/time_range/time_range_dialog.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "profile/window_definition_util.h"
#include "resources/common_resources.h"
#include "ui/common/client_utils.h"

#if defined(UI_QT)
#include "aui/severity_colors.h"
#include "events/qt/alarm_footer.h"
#include "events/qt/area_sidebar.h"
#include "events/qt/event_filter_bar.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#endif

namespace {

// Message-box title. A function, not a constant: Translate() reads the
// installed catalog and so needs a running QApplication.
std::u16string FilterTitle() {
  return Translate("Filter");
}

struct EventTableModelHolder {
  EventTableModelHolder(const ControllerContext& context,
                        LocalEvents& local_events,
                        bool is_panel)
      : current_event_model{context.node_event_provider_},
        historical_event_model{context.executor_, context.history_service_},
        local_event_model{local_events},
        event_table_model{{context.executor_, context.node_service_,
                           current_event_model, historical_event_model,
                           local_event_model, is_panel}} {}

  CurrentEventModel current_event_model;
  HistoricalEventModel historical_event_model;
  LocalEventModel local_event_model;
  EventTableModel event_table_model;
};

std::shared_ptr<EventTableModel> CreateEventTableModel(
    const ControllerContext& context,
    LocalEvents& local_events,
    bool is_panel) {
  auto holder =
      std::make_shared<EventTableModelHolder>(context, local_events, is_panel);
  return {holder, &holder->event_table_model};
}

scada::EventSeverity ParseSeverity(std::u16string_view str) {
  unsigned severity = 0;
  if (!Parse(str, severity) || severity > scada::kSeverityMax) {
    throw ResourceError{u16format(Translate("Enter a number from {} to {}."),
                                  scada::kSeverityMin, scada::kSeverityMax)};
  }

  // TODO: Checked cast.
  return static_cast<scada::EventSeverity>(severity);
}

}  // namespace

// EventView

EventView::EventView(const ControllerContext& context,
                     LocalEvents& local_events,
                     bool is_panel,
                     bool audit_only)
    : ControllerContext{context},
      is_panel_{is_panel},
      audit_only_{audit_only},
      local_events_{local_events},
      model_{CreateEventTableModel(context, local_events, is_panel)} {
  const scada::aui::TableColumn kEventViewColumns[] = {
      {EventColumnTime, Translate("Time"), 150, scada::aui::TableColumn::LEFT,
       scada::aui::TableColumn::DataType::DateTime},
      {EventColumnItem, Translate("Item"), 170, scada::aui::TableColumn::LEFT},
      // Wide enough for the named alarm band, which spells the severity out
      // instead of leaving colour as its only signal.
      {EventColumnSeverity, Translate("Severity"), 165,
       scada::aui::TableColumn::RIGHT},
      {EventColumnValue, Translate("Value"), 100,
       scada::aui::TableColumn::RIGHT,
       scada::aui::TableColumn::DataType::General,
       /*monospace=*/true},
      {EventColumnMessage, Translate("Message"), 300,
       scada::aui::TableColumn::LEFT},
      {EventColumnUser, Translate("User"), 100, scada::aui::TableColumn::LEFT},
      {EventColumnAckUser, Translate("Acknowledged By"), 100,
       scada::aui::TableColumn::LEFT},
      {EventColumnAckTime, Translate("Acknowledge Time"), 150,
       scada::aui::TableColumn::LEFT,
       scada::aui::TableColumn::DataType::DateTime},
  };

  size_t count = std::size(kEventViewColumns);
  if (is_panel)
    count -= 2;

  std::vector<scada::aui::TableColumn> columns(kEventViewColumns,
                                               kEventViewColumns + count);
  // The exports carry the journal's record columns; the leading pending-dot
  // marker added below is display chrome and stays out of them.
  export_columns_ = columns;
  // Leading pending marker: a severity-coloured dot on every unacknowledged row
  // (see EventColumnUnacked), first so the actionable rows read at a glance.
  columns.insert(columns.begin(), {EventColumnUnacked, u"", 28,
                                   scada::aui::TableColumn::CENTER});

  // cppcheck-suppress noCopyConstructor
  // cppcheck-suppress noOperatorEq
  table_ = new scada::aui::Table{model_, std::move(columns), true};

#if defined(UI_QT)
  // Newest-first on the Time column, which the leading dot column precedes.
  table_->sortByColumn(1, Qt::DescendingOrder);
#endif

  table_->SetContextMenuHandler([this](const scada::aui::Point& point) {
    // Show the event view's own AUI menu model (works on Windows and macOS)
    // rather than the Windows-only `IDR_EVENT_POPUP` resource menu.
    controller_delegate_.ShowPopupMenu(&event_menu_model_.model(), point, true);
  });

  table_->SetSelectionChangeHandler([this] { OnSelectionChanged(); });

  table_->SetDoubleClickHandler([this] { AcknowledgeSelection(); });

  table_->SetKeyPressHandler(
      [this](scada::aui::KeyCode key_code) { return OnKeyPressed(key_code); });

  selection_.multiple_handler = [this] { return GetSelectedNodeIds(); };

  command_registry_.AddCommand(
      Command{ID_ACKNOWLEDGE_CURRENT}
          .set_execute_handler([this] { AcknowledgeSelection(); })
          .set_enabled_handler([this] { return CanAcknowledgeSelection(); })
          .set_disabled_reason_handler([this] {
            return table_->GetSelectedRows().empty()
                       ? Translate("Select an event to acknowledge")
                       : Translate(
                             "The selected events are already "
                             "acknowledged");
          }));

  command_registry_.AddCommand(
      Command{ID_ACKNOWLEDGE_ALL}
          .set_execute_handler([this] {
            node_event_provider_.AcknowledgeAllEvents();
            local_events_.AcknowledgeAll();
          })
          .set_enabled_handler([this] {
            return !node_event_provider_.unacked_events().empty() ||
                   !local_events_.events().empty();
          })
          .set_disabled_reason_handler([] {
            return Translate("Nothing is waiting to be acknowledged");
          }));

  command_registry_.AddCommand(
      Command{ID_UNACKNOWLEDGED_ONLY}
          .set_execute_handler([this] {
            model_->SetUnacknowledgedOnly(!model_->unacknowledged_only());
          })
          .set_checked_handler(
              [this] { return model_->unacknowledged_only(); }));

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
  // The whole selection goes in one call: the model resolves every target
  // before acknowledging any of them, because acknowledging can remove rows or
  // regroup them and leave row indices stale mid-loop.
  model_->AcknowledgeRows(table_->GetSelectedRows());
}

void EventView::OnSelectionChanged() {
  auto rows = table_->GetSelectedRows();
  if (rows.empty())
    selection_.Clear();
  else if (rows.size() >= 2)
    selection_.SelectMultiple();
  else {
    // Publish the event itself, with its source as the node selection —
    // node-scoped commands stay enabled and event-shaped surfaces (the
    // Inspector's alarm card) read the alarm.
    const scada::Event& event = model_->event_at(rows.front());
    selection_.SelectEvent(event, node_service_.GetNode(event.source_node_id));
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

std::unique_ptr<UiView> EventView::Init(const WindowDefinition& definition) {
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
    // "Current" mode (the Overview page's alarm table, ID_OPEN_EVENTS): the
    // journal opens scoped to actionable events — the unacknowledged-only
    // filter pre-set rather than a separate surface, so the operator can
    // widen the scope from the filter bar. Previously this item was written
    // by every "current events" open path but consumed by nothing.
    if (const WindowItem* mode = definition.FindItem("mode");
        mode && mode->attributes.is_string() &&
        mode->attributes.as_string() == "Current") {
      model_->SetUnacknowledgedOnly(true);
    }
    // The Audit log is this journal scoped to the AuditEventType subtree.
    // Deliberately NOT also unacknowledged-only: an audit trail records what
    // happened, and acknowledgement has no meaning for it.
    if (audit_only_) {
      model_->SetAuditOnly(true);
    }
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

#if defined(UI_QT)
  // The journal filter bar: discoverable chrome for the filters, complementing
  // the right-click context menu (which is cross-platform via
  // `event_menu_model_`). Only on the full journal, not the docked panel.
  if (!is_panel_) {
    auto* container = new QWidget;
    auto* layout = new QVBoxLayout{container};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    // The area currently applied through the sidebar, so switching areas only
    // touches that scope and leaves other filter items intact.
    auto applied_area = std::make_shared<std::optional<scada::NodeId>>();
    auto apply_area = [this,
                       applied_area](const std::optional<scada::NodeId>& area) {
      if (*applied_area == area)
        return;
      if (*applied_area)
        model_->RemoveFilteredItem(**applied_area);
      if (area)
        model_->AddFilteredItem(*area);
      *applied_area = area;
      controller_delegate_.SetTitle(MakeTitle());
    };
    layout->addWidget(MakeEventFilterBar(EventFilterBarContext{
        .executor = executor_,
        .node_service = node_service_,
        // The audit log hides the unacknowledged-only toggle for the same
        // reason it hides the footer: acknowledgement has no meaning here.
        .show_unacknowledged_only = !audit_only_,
        .unacknowledged_only = model_->unacknowledged_only(),
        .severity_min = model_->severity_min(),
        .severity_max = scada::kSeverityMax,
        .time_range = model_->time_range(),
        .on_unacknowledged_only =
            [this](bool value) { model_->SetUnacknowledgedOnly(value); },
        .on_severity_min =
            [this](unsigned severity) {
              SetSeverityMin(static_cast<scada::EventSeverity>(severity));
            },
        .on_time_range =
            [this](const scada::RelativeTimeRange& time_range) {
              SetTimeRange(time_range);
            },
    }));
    // Areas sidebar beside the journal: every top-level area with its
    // unacknowledged count, driving the same area-filter scope the filter
    // bar's dropdown used to.
    auto* body = new QWidget;
    auto* body_layout = new QHBoxLayout{body};
    body_layout->setContentsMargins(0, 0, 0, 0);
    body_layout->setSpacing(0);
    body_layout->addWidget(MakeEventAreaSidebar(EventAreaSidebarContext{
        .executor = executor_,
        .model = *model_,
        .browse_areas = [this] { return BrowseEventAreas(node_service_); },
        .counts =
            [this](std::span<const scada::NodeId> areas) {
              return model_->CountUnacknowledgedByArea(areas);
            },
        .on_area = apply_area}));
    body_layout->addWidget(table_->CreateParentIfNecessary(), 1);
    layout->addWidget(body, 1);
    // Alarm footer: the displayed backlog summary plus Acknowledge-all, so
    // the journal's actionable state and the action on it sit together.
    //
    // Omitted on the audit log. An audit entry records that something
    // happened; there is nothing to acknowledge, and offering the action —
    // or an unacknowledged backlog count over a trail that has none — would
    // describe the surface as something it is not.
    if (!audit_only_) {
      layout->addWidget(MakeAlarmFooter(AlarmFooterContext{
          .model = *model_,
          .summary = [this] { return model_->GetAlarmSummary(); },
          .acknowledge_all = [this]() -> CommandHandler* {
            return command_registry_.GetCommandHandler(ID_ACKNOWLEDGE_ALL);
          }}));
    }
    return std::unique_ptr<UiView>{container};
  }
#endif

  return std::unique_ptr<UiView>{table_->CreateParentIfNecessary()};
}

bool EventView::OnKeyPressed(scada::aui::KeyCode key_code) {
  switch (key_code) {
    case scada::aui::KeyCode::Escape:
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
  // Expanded for the same reason as GetExportData() above.
  int rows = expanded_model_.GetRowCount();
  if (!rows) {
    ShowMessageBox(executor_, dialog_service_, Translate("No data to export."),
                   Translate("Export"), MessageBoxMode::Info);
    return;
  }

  try {
    ExcelSheetModel sheet{rows + 1, EventColumnCount};

    // The record columns only — no display-only pending-dot column.
    const auto& columns = export_columns_;

    for (size_t i = 0; i < columns.size(); ++i)
      sheet.SetData(1, i + 1, UtfConvert<wchar_t>(columns[i].title));

    for (int row = 0; row < expanded_model_.GetRowCount(); ++row) {
      for (size_t col = 0; col < columns.size(); ++col) {
        auto text = expanded_model_.GetCellText(row, columns[col].id);
        sheet.SetData(2 + row, col + 1, UtfConvert<wchar_t>(text));
      }
    }

    Excel excel;
    excel.NewWorkbook();
    excel.NewSheet(sheet);
    excel.SetVisible();

  } catch (HRESULT /*err*/) {
    ShowMessageBox(executor_, dialog_service_, Translate("Export error."),
                   Translate("Export"), MessageBoxMode::Error);
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

scada::RelativeTimeRange EventView::GetTimeRange() const {
  return model_->time_range();
}

CommandHandler* EventView::GetCommandHandler(unsigned command_id) {
  return command_registry_.GetCommandHandler(command_id);
}

void EventView::SetTimeRange(const scada::RelativeTimeRange& time_range) {
  model_->SetTimeRange(time_range);
  controller_delegate_.SetTitle(MakeTitle());
}

void EventView::SelectSeverity() {
  CoSpawn(executor_, [this]() -> Awaitable<void> {
    co_await SelectSeverityAsync();
    co_return;
  });
}

Awaitable<void> EventView::SelectSeverityAsync() {
  unsigned initial_severity = model_->current_events()
                                  ? node_event_provider_.severity_min()
                                  : model_->severity_min();
  auto prompt = Translate("Minimum severity threshold (0 = all events):");

  // Wait for the prompt dialog. A user cancel surfaces as a rejection here,
  // matching the old ignored asynchronous result.
  auto text = co_await RunPromptDialog(dialog_service_, prompt,
                                       /*title=*/FilterTitle(),
                                       WideFormat(initial_severity));

  // Parse + apply. Preserve the original behavior where a bad value
  // pops up an error message box via `ShowResourceError` and then
  // rejects the coroutine with the same exception.
  // MSVC rejects `co_await` inside a `catch` block, so the exception is
  // captured here and awaited after the `try` has unwound.
  std::exception_ptr parse_error;
  try {
    SetSeverityMin(ParseSeverity(text));
  } catch (...) {
    parse_error = std::current_exception();
  }
  if (parse_error) {
    co_await ShowResourceError<void>(dialog_service_, /*title=*/FilterTitle(),
                                     parse_error);
  }
  co_return;
}

void EventView::SetSeverityMin(scada::EventSeverity severity) {
  if (model_->current_events()) {
    // 0 means "all events" in the UI; the fetcher's valid floor is
    // kSeverityMin (1), which matches every event on the 1-1000 scale
    // (0 is not a valid severity — OPC UA Part 5 §6.4.2).
    node_event_provider_.SetSeverityMin(severity == 0 ? scada::kSeverityMin
                                                      : severity);
    profile_.NotifyChange();
  } else {
    model_->SetSeverityMin(severity);
    controller_delegate_.SetTitle(MakeTitle());
  }
}

NodeIdSet EventView::GetSelectedNodeIds() const {
  NodeIdSet node_ids;
  for (auto row : table_->GetSelectedRows()) {
    const scada::Event& event = model_->event_at(row);
    if (!event.source_node_id.is_null())
      node_ids.insert(event.source_node_id);
  }
  return node_ids;
}

TimeModel* EventView::GetTimeModel() {
  return model_->current_events() ? nullptr : this;
}

ExportModel::ExportData EventView::GetExportData() {
  // Both forms: the rows as displayed, and the same events expanded out of
  // their flood groups. A flood group is a way of *displaying* a wall of
  // repeats, not of recording it, so the record-shaped consumers (spreadsheet,
  // printout) take the expanded form and the CSV dialog offers the choice.
  // `export_columns_` excludes the display-only pending-dot column.
  return TableExportData{*model_, export_columns_, /*row_range=*/std::nullopt,
                         &expanded_model_};
}
