#include "components/debugger/request_table_model.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "scada/session_service.h"

RequestTableModel::RequestTableModel(scada::SessionService& session_service) {
  if (auto* session_debugger = session_service.GetSessionDebugger()) {
    request_event_connection_ = session_debugger->SubscribeRequestEvents(
        [this](const scada::SessionDebugger::RequestEvent& event) {
          UpdateRequest(event);
        });
  }
}

int RequestTableModel::GetRowCount() {
  return static_cast<int>(requests_.size());
}

void RequestTableModel::GetCell(aui::TableCell& cell) {
  const auto& request = requests_[cell.row];

  switch (cell.column_id) {
    case 0:
      // ID
      cell.text = base::NumberToString16(request.id);
      break;
    case 1:
      // Phase
      cell.text = base::ASCIIToUTF16(ToString(request.phase));
      break;
    case 2:
      // Title
      cell.text = base::ASCIIToUTF16(request.title);
      break;
  }
}

void RequestTableModel::UpdateRequest(
    const scada::SessionDebugger::RequestEvent& event) {
  bool completed = event.phase == RequestPhase::Succeeded ||
                   event.phase == RequestPhase::Failed;
  if (auto i = request_id_to_index_.find(event.id);
      i != request_id_to_index_.end()) {
    auto index = i->second;
    auto& existing_request = requests_[index];
    existing_request.phase = event.phase;
    if (!event.title.empty()) {
      existing_request.title = event.title;
    }
    if (!event.body.empty()) {
      existing_request.body = event.body;
    }
    if (!event.response_body.empty()) {
      existing_request.response_body = event.response_body;
    }
    if (completed) {
      request_id_to_index_.erase(i);
    }
    NotifyItemsChanged(index, 1);
  } else {
    auto index = static_cast<int>(requests_.size());
    NotifyItemsAdding(index, 1);
    requests_.emplace_back(
        Request{.id = event.id,
                .phase = event.phase,
                .title = std::string{event.title},
                .body = std::string{event.body},
                .response_body = std::string{event.response_body}});
    if (!completed) {
      request_id_to_index_.try_emplace(event.id, index);
    }
    NotifyItemsAdded(index, 1);
  }
}
