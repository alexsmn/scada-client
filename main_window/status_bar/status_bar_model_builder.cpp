#include "main_window/status_bar/status_bar_model_builder.h"

#include "aui/models/status_bar_model_impl.h"
#include "events/node_event_provider.h"
#include "main_window/status_bar/capture_status_provider.h"
#include "main_window/status_bar/event_status_provider.h"
#include "main_window/status_bar/selection_status_provider.h"
#include "main_window/status_bar/session_status_provider.h"
#include "main_window/status_bar/user_status_provider.h"

std::shared_ptr<scada::aui::StatusBarModel> StatusBarModelBuilder::Build() {
  auto model = std::make_shared<scada::aui::StatusBarModelImpl>();

  // Event count and min severity.

  auto event_status_provider = std::make_shared<EventStatusProvider>(
      node_event_provider_, local_events_, profile_);

  int event_count_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&EventStatusProvider::GetEventCountText,
                                        event_status_provider),
       .size = 100});

  int severity_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&EventStatusProvider::GetSeverityText,
                                        event_status_provider),
       .size = 100});

  // Highest active alarm severity, coloured from the severity single source.
  // Empty when nothing is asserted.
  int highest_severity_pane_index = model->AddPane(
      {.text_provider = std::bind_front(
           &EventStatusProvider::GetHighestSeverityText, event_status_provider),
       .color_provider =
           std::bind_front(&EventStatusProvider::GetHighestSeverityColor,
                           event_status_provider),
       .size = 130});

  // The unacknowledged-alarm count for chrome that shows an unread badge (the
  // activity rail); refreshed together with the panes below.
  model->SetAlarmCountProvider(std::bind_front(
      &EventStatusProvider::GetAlarmCount, event_status_provider));

  // Per-severity unacknowledged counts for the live KPI tiles.
  model->SetSeverityCountProvider(std::bind_front(
      &EventStatusProvider::GetSeverityCount, event_status_provider));

  event_status_provider->Init([model, event_count_pane_index,
                               severity_pane_index,
                               highest_severity_pane_index] {
    model->NotifyPanesChanged(event_count_pane_index);
    model->NotifyPanesChanged(severity_pane_index);
    model->NotifyPanesChanged(highest_severity_pane_index);
  });

  // An armed frame capture. Placed before the user pane so it sits next to the
  // alarm cells — it is a "something is running" state, not an identity.

  auto capture_status_provider =
      std::make_shared<CaptureStatusProvider>(frame_capture_registry_);

  int capture_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&CaptureStatusProvider::GetText,
                                        capture_status_provider),
       .color_provider = std::bind_front(&CaptureStatusProvider::GetColor,
                                         capture_status_provider),
       .size = 160});

  capture_status_provider->Init([model, capture_pane_index] {
    model->NotifyPanesChanged(capture_pane_index);
  });

  // User.

  auto user_status_provider = std::make_shared<UserStatusProvider>(
      executor_, node_service_, session_service_);

  int user_pane_index =
      model->AddPane({.text_provider = std::bind_front(
                          &UserStatusProvider::GetText, user_status_provider),
                      .size = 160});

  user_status_provider->Init(
      [model, user_pane_index] { model->NotifyPanesChanged(user_pane_index); });

  // Connection state and pings.

  auto session_status_provider = std::make_shared<SessionStatusProvider>(
      executor_, session_service_, local_events_);

  int connection_pane_index =
      model->AddPane({.text_provider = std::bind_front(
                          &SessionStatusProvider::GetConnectionStateText,
                          session_status_provider),
                      .size = 100});

  int ping_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&SessionStatusProvider::GetPingText,
                                        session_status_provider),
       .color_provider = std::bind_front(&SessionStatusProvider::GetPingColor,
                                         session_status_provider),
       // Wider than the bare "<server>: N ms" it used to hold: a stalled
       // session appends a marker, and the number grows into five digits.
       .size = 180});

  // What the operator has selected on a schematic display. Placed between the
  // ping and the endpoint, which is where the mockup's status strip carries it
  // -- after the connection cells, ahead of the cells naming the installation.
  // Empty until something is selected, so the strip below is unchanged for
  // everyone who never opens a display.

  auto selection_status_provider =
      std::make_shared<SelectionStatusProvider>(display_selection_registry_);

  int selection_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&SelectionStatusProvider::GetText,
                                        selection_status_provider),
       .size = 160});

  selection_status_provider->Init([model, selection_pane_index] {
    model->NotifyPanesChanged(selection_pane_index);
  });

  int endpoint_pane_index = model->AddPane(
      {.text_provider = std::bind_front(&SessionStatusProvider::GetEndpointText,
                                        session_status_provider),
       .size = 200});

  session_status_provider->Init(
      [model, connection_pane_index, ping_pane_index, endpoint_pane_index] {
        model->NotifyPanesChanged(connection_pane_index);
        model->NotifyPanesChanged(ping_pane_index);
        model->NotifyPanesChanged(endpoint_pane_index);
      });

  return model;
}