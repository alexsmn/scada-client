#include "events/event_module.h"

#include "base/value_util.h"
#include "controller/command_registry.h"
#include "controller/controller_registry.h"
#include "core/selection_command_context.h"
#include "events/event_fetcher.h"
#include "events/event_fetcher_builder.h"
#include "events/event_view.h"
#include "events/local_events.h"
#include "main_window/main_window_interface.h"
#include "main_window/opened_view_interface.h"
#include "profile/profile.h"

namespace {

constexpr WindowInfo kEventWindowInfo = {
    .command_id = ID_EVENT_VIEW,
    .name = "Event",
    .title = u"�������",
    .flags = WIN_SING | WIN_DOCKB | WIN_CAN_PRINT,
    .size = {800, 200}};

constexpr WindowInfo kEventJournalWindowInfo = {
    .command_id = ID_EVENT_JOURNAL_VIEW,
    .name = "EventJournal",
    .title = u"������ �������",
    .flags = WIN_INS | WIN_CAN_PRINT};

}  // namespace

EventModule::EventModule(EventModuleContext&& context)
    : EventModuleContext(std::move(context)) {
  event_fetcher_ =
      EventFetcherBuilder{
          .executor_ = executor_, .logger_ = logger_, .services_ = services_}
          .Build();

  // TODO: Checked cast.
  event_fetcher_->SetSeverityMin(static_cast<scada::EventSeverity>(
      GetInt(profile_.data(), "severityMin",
             static_cast<unsigned>(scada::kSeverityMin))));

  local_events_ = std::make_unique<LocalEvents>();

  controller_registry_.AddControllerFactory(
      kEventWindowInfo,
      [&local_events = *local_events_](const ControllerContext& context) {
        return std::make_unique<EventView>(context, local_events,
                                           /*is_panel=*/true);
      });

  controller_registry_.AddControllerFactory(
      kEventJournalWindowInfo,
      [&local_events = *local_events_](const ControllerContext& context) {
        return std::make_unique<EventView>(context, local_events,
                                           /*is_panel=*/false);
      });

  profile_.RegisterSerializer([this](boost::json::value& data) {
    SetKey(data, "severityMin",
           static_cast<int>(event_fetcher_->severity_min()));
  });

  AddOpenCommand(ID_OPEN_EVENTS, kEventJournalWindowInfo, "Current");
  AddOpenCommand(ID_HISTORICAL_EVENTS, kEventJournalWindowInfo);
}

EventModule::~EventModule() {}

NodeEventProvider& EventModule::node_event_provider() {
  return *event_fetcher_;
}

void EventModule::AddOpenCommand(unsigned command_id,
                                 const WindowInfo& window_info,
                                 const std::string_view& mode) {
  selection_commands_.AddCommand(
      {.command_id = command_id,
       .execute_handler =
           [&window_info, mode](const SelectionCommandContext& context) {
             context.opened_view
                 .GetOpenWindowDefinition(&window_info)
                 // TODO: `main_window` must be a weak pointer.
                 .then([&main_window = context.main_window,
                        mode](const WindowDefinition& window_def) {
                   auto new_window_def = window_def;
                   if (!mode.empty()) {
                     new_window_def.AddItem("mode", mode);
                   }
                   main_window.OpenView(new_window_def);
                 });
           },
       .available_handler =
           [](const SelectionCommandContext& context) {
             return !context.selection.empty();
           }});
}
