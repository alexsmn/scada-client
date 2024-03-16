#include "components/events/event_module.h"

#include "base/value_util.h"
#include "events/event_fetcher.h"
#include "events/event_fetcher_builder.h"
#include "profile/profile.h"
#include "services/local_events.h"

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