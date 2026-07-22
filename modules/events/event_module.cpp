#include "events/event_module.h"

#include <algorithm>

#include "aui/translation.h"
#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/value_util.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "core/global_command_context.h"
#include "core/selection_command_context.h"
#include "events/event_fetcher.h"
#include "events/event_fetcher_builder.h"
#include "events/event_view.h"
#include "events/local_events.h"
#include "events/node_event_provider.h"
#include "main_window/main_window_interface.h"
#include "main_window/opened_view/opened_view_interface.h"
#include "profile/profile.h"

namespace {

constexpr WindowInfo kEventWindowInfo = {
    .command_id = ID_EVENT_VIEW,
    .name = "Event",
    .title = u"Events",
    .flags = WIN_SING | WIN_DOCKB | WIN_CAN_PRINT,
    .size = {800, 200}};

constexpr WindowInfo kEventJournalWindowInfo = {
    .command_id = ID_EVENT_JOURNAL_VIEW,
    .name = "EventJournal",
    .title = u"Event Journal",
    .flags = WIN_INS | WIN_CAN_PRINT};

Awaitable<void> OpenWindowDefinition(
    std::string_view mode,
    MainWindowInterface& main_window,
    Awaitable<WindowDefinition> window_def_awaitable) {
  auto window_def = co_await std::move(window_def_awaitable);
  if (!mode.empty()) {
    window_def.AddItem("mode", mode);
  }
  co_await main_window.OpenView(window_def);
  co_return;
}

}  // namespace

EventModule::EventModule(EventModuleContext&& context)
    : EventModuleContext(std::move(context)) {
  event_fetcher_ =
      EventFetcherBuilder{
          .executor_ = executor_, .logger_ = logger_, .services_ = services_}
          .Build();

  // TODO: Checked cast.
  unsigned severity_min = GetInt(profile_.data(), "severityMin",
                                 static_cast<unsigned>(scada::kSeverityMin));
  // Profiles written before ADR 0005 phase 1 stored the 0-100 severity
  // scale; the severityScale marker (written below) distinguishes them.
  // Rescale so a saved filter keeps its meaning instead of silently
  // loosening on the 1-1000 scale.
  if (GetInt(profile_.data(), "severityScale", 100) != 1000) {
    severity_min = std::clamp(severity_min * 10, 1u, 1000u);
  }
  event_fetcher_->SetSeverityMin(
      static_cast<scada::EventSeverity>(severity_min));

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
    SetKey(data, "severityScale", 1000);
  });

  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_OPEN_EVENTS}.set_execute_handler(
          [executor = executor_](const GlobalCommandContext& context) {
            CoSpawn(executor,
                    [&main_window = context.main_window]() -> Awaitable<void> {
                      WindowDefinition window_def{kEventJournalWindowInfo};
                      window_def.AddItem("mode", "Current");
                      co_await main_window.OpenView(window_def);
                    });
          }));
  AddOpenCommand(ID_OPEN_EVENTS, kEventJournalWindowInfo, "Current");
  AddOpenCommand(ID_HISTORICAL_EVENTS, kEventJournalWindowInfo);
  global_commands_.AddCommand(BasicCommand<GlobalCommandContext>{
      .command_id = ID_ACKNOWLEDGE_ALL,
      .execute_handler =
          [this](const GlobalCommandContext&) {
            node_event_provider().AcknowledgeAllEvents();
            local_events_->AcknowledgeAll();
          },
      .enabled_handler =
          [this](const GlobalCommandContext&) {
            return !node_event_provider().unacked_events().empty() ||
                   !local_events_->events().empty();
          }});
  selection_commands_.AddCommand(BasicCommand<SelectionCommandContext>{
      .command_id = ID_ACKNOWLEDGE_CURRENT,
      .execute_handler =
          [this](const SelectionCommandContext& context) {
            node_event_provider().AcknowledgeItemEvents(
                context.selection.node().node_id());
          },
      .enabled_handler =
          [](const SelectionCommandContext& context) {
            return context.selection.timed_data().alerting();
          },
      .available_handler =
          [](const SelectionCommandContext& context) {
            return context.selection.timed_data().connected();
          }});

  ui_command_registry_.AddAction(Action{.command_id_ = ID_HISTORICAL_EVENTS,
                                        .category_ = CATEGORY_OPEN,
                                        .title_ = Translate("Events"),
                                        .image_id_ = IDB_OPEN_EVENTS,
                                        .flags_ = Action::ALWAYS_VISIBLE});
  ui_command_registry_.AddAction(Action{.command_id_ = ID_ACKNOWLEDGE_CURRENT,
                                        .category_ = CATEGORY_ITEM,
                                        .title_ = Translate("Acknowledge")});
  ui_command_registry_.AddAction(Action{.command_id_ = ID_ACKNOWLEDGE_ALL,
                                        .category_ = CATEGORY_VIEW,
                                        .title_ = Translate("Acknowledge All"),
                                        .image_id_ = IDB_ACKNOWLEDGE_ALL});
  ui_command_registry_.AddAction(Action{.command_id_ = ID_SEVERITY_CUSTOM,
                                        .category_ = CATEGORY_VIEW,
                                        .title_ = Translate("Severity..."),
                                        .short_title_ = Translate("Severity")});
  ui_command_registry_.AddAction(Action{.command_id_ = ID_EVENT_VIEW,
                                        .category_ = CATEGORY_VIEW,
                                        .title_ = Translate("Event Panel"),
                                        .image_id_ = ID_EVENT_VIEW});
  ui_command_registry_.AddAction(Action{.command_id_ = ID_CURRENT_EVENTS,
                                        .category_ = CATEGORY_PERIOD,
                                        .title_ = Translate("Current"),
                                        .flags_ = Action::CHECKABLE});

  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 110,
                                    .command_id = ID_EVENT_VIEW,
                                    .title = Translate("Events"),
                                    .checkable = true});
  ui_command_registry_.AddMenuItem({.menu_id = MainMenuId::More,
                                    .order = 160,
                                    .command_id = ID_EVENT_JOURNAL_VIEW,
                                    .title = Translate("Event Journal"),
                                    .checkable = true});
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
           [&window_info, mode,
            executor = executor_](const SelectionCommandContext& context) {
             auto window_def =
                 context.opened_view.GetOpenWindowDefinition(&window_info);
             CoSpawn(executor, [mode, &main_window = context.main_window,
                                window_def = std::move(window_def)]() mutable {
               return OpenWindowDefinition(mode, main_window,
                                           std::move(window_def));
             });
           },
       .available_handler =
           [](const SelectionCommandContext& context) {
             return !context.selection.empty();
           }});
}
