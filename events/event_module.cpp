#include "events/event_module.h"

#include "base/value_util.h"
#include "controller/controller_registry.h"
#include "events/event_fetcher.h"
#include "events/event_fetcher_builder.h"
#include "events/event_view.h"
#include "events/local_events.h"
#include "profile/profile.h"

namespace {

constexpr WindowInfo kEventWindowInfo = {
    .command_id = ID_EVENT_VIEW,
    .name = "Event",
    .title = u"События",
    .flags = WIN_SING | WIN_DOCKB | WIN_CAN_PRINT,
    .size = {800, 200}};

constexpr WindowInfo kEventJournalWindowInfo = {
    .command_id = ID_EVENT_JOURNAL_VIEW,
    .name = "EventJournal",
    .title = u"Журнал событий",
    .flags = WIN_INS | WIN_CAN_PRINT};

}  // namespace

EventModule::EventModule(EventModuleContext&& context)
    : EventModuleContext(std::move(context)) {
  event_fetcher_ =
      EventFetcherBuilder{
          .executor_ = executor_, .logger_ = logger_, .services_ = services_}
          .Build();

  // TODO: Checked cast.
  scada::EventSeverity severity_min = static_cast<scada::EventSeverity>(
      GetInt(profile_.data(), "severityMin",
             static_cast<unsigned>(scada::kSeverityMin)));

  event_fetcher_->SetSeverityMin(severity_min);

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

  profile_.RegisterSerializer([this](base::Value& data) {
    SetKey(data, "severityMin",
           static_cast<int>(event_fetcher_->severity_min()));
  });

  local_events_ = std::make_unique<LocalEvents>();
}

EventModule::~EventModule() {}

NodeEventProvider& EventModule::node_event_provider() {
  return *event_fetcher_;
}