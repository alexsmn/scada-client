#include "main_window/status/status_bar_model_builder.h"

#include "events/node_event_provider.h"
#include "main_window/status/event_status_provider.h"
#include "main_window/status/session_status_provider.h"
#include "main_window/status/status_bar_model_impl.h"
#include "main_window/status/user_status_provider.h"

std::shared_ptr<aui::StatusBarModel> StatusBarModelBuilder::Build() {
  auto model = std::make_shared<StatusBarModelImpl>();

  // Event count and min severity.

  auto event_status_provider =
      std::make_shared<EventStatusProvider>(node_event_provider_, profile_);

  int event_count_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&EventStatusProvider::GetEventCountText,
                                        event_status_provider),
       .size = 100});

  int severity_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&EventStatusProvider::GetSeverityText,
                                        event_status_provider),
       .size = 100});

  event_status_provider->Init(
      [model, event_count_pane_index, severity_pane_index] {
        model->NotifyPanesChanged(event_count_pane_index);
        model->NotifyPanesChanged(severity_pane_index);
      });

  // User.

  auto user_status_provider = std::make_shared<UserStatusProvider>(
      executor_, node_service_, session_service_);

  int user_pane_index =
      model->AddPane({.text_provider = std::bind_front(
                          &UserStatusProvider::GetText, user_status_provider),
                      .size = 100});

  user_status_provider->Init(
      [model, user_pane_index] { model->NotifyPanesChanged(user_pane_index); });

  // Connection state and pings.

  auto session_status_provider =
      std::make_shared<SessionStatusProvider>(executor_, session_service_);

  int connection_pane_index =
      model->AddPane({.text_provider = std::bind_front(
                          &SessionStatusProvider::GetConnectionStateText,
                          session_status_provider),
                      .size = 100});

  int ping_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&SessionStatusProvider::GetPingText,
                                        session_status_provider),
       .size = 120});

  session_status_provider->Init(
      [model, connection_pane_index, ping_pane_index] {
        model->NotifyPanesChanged(connection_pane_index);
        model->NotifyPanesChanged(ping_pane_index);
      });

  return model;
}