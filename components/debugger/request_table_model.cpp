#include "components/debugger/request_table_model.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time_utils.h"
#include "scada/session_service.h"

namespace {
const size_t kMaxRequests = 1000;
}

std::string DumpRequest(const RequestTableModel::Request& request) {
  return std::format(
      "Request ID: {}\nPhase: {}\nStart time: {}\n\nRequest "
      "body:\n{}\nResponse body:\n{}",
      request.request_id, ToString(request.phase), request.start_time,
      request.body, request.response_body);
}

// RequestTableModel

RequestTableModel::RequestTableModel(scada::SessionService& session_service) {
  if (auto* session_debugger = session_service.GetSessionDebugger()) {
    request_event_connection_ = session_debugger->SubscribeRequestEvents(
        std::bind_front(&RequestTableModel::ProcessRequestEvent, this));
  }
}

int RequestTableModel::GetRowCount() {
  return static_cast<int>(requests_.size());
}

void RequestTableModel::GetCell(aui::TableCell& cell) {
  const auto& request = requests_[cell.row];

  switch (cell.column_id) {
    case 0:
      cell.text = base::NumberToString16(request.request_id);
      break;
    case 1:
      cell.text = base::ASCIIToUTF16(ToString(request.phase));
      break;
    case 2:
      cell.text = base::ASCIIToUTF16(std::format("{}", request.start_time));
      break;
    case 3:
      if (request.finish_time != RequestTime{}) {
        auto duration = request.finish_time - request.start_time;
        cell.text =
            base::ASCIIToUTF16(std::format("{} ms", InMilliseconds(duration)));
      }
      break;
    case 4:
      cell.text = base::ASCIIToUTF16(request.title);
      break;
  }
}

void RequestTableModel::ProcessRequestEvent(
    const scada::SessionDebugger::RequestEvent& event) {
  if (UpdateRunningRequest(event)) {
    return;
  }

  AddRequest(event);
  RemoveOldRequests();
}

bool RequestTableModel::UpdateRunningRequest(
    const scada::SessionDebugger::RequestEvent& event) {
  auto i = running_request_id_to_index_.find(event.request_id);
  if (i == running_request_id_to_index_.end()) {
    return false;
  }

  int index = i->second;
  Request& request = requests_[index];

  UpdateRequestFromEvent(request, event);

  if (!IsRunningRequest(request.phase)) {
    request.finish_time = std::chrono::system_clock::now();
    running_request_id_to_index_.erase(i);
  }

  NotifyItemsChanged(index, 1);
  return true;
}

void RequestTableModel::AddRequest(
    const scada::SessionDebugger::RequestEvent& event) {
  auto index = static_cast<int>(requests_.size());

  ScopedItemsAdding items_adding{*this, index, 1};

  auto& request = requests_.emplace_back(
      Request{.request_id = event.request_id,
              .phase = event.phase,
              .start_time = std::chrono::system_clock::now(),
              .title = std::string{event.title},
              .body = std::string{event.body},
              .response_body = std::string{event.response_body}});

  if (IsRunningRequest(event.phase)) {
    running_request_id_to_index_.try_emplace(event.request_id, index);
  } else {
    request.finish_time = request.start_time;
  }
}

void RequestTableModel::RemoveOldRequests() {
  if (requests_.size() <= kMaxRequests) {
    return;
  }

  int remove_count = requests_.size() - kMaxRequests;
  ScopedItemsRemoving items_removing{*this, 0, remove_count};
  requests_.erase(requests_.begin(), requests_.begin() + remove_count);
}

// static
bool RequestTableModel::IsRunningRequest(RequestPhase phase) {
  return phase != RequestPhase::Succeeded && phase != RequestPhase::Failed;
}

// static
void RequestTableModel::UpdateRequestFromEvent(
    Request& request,
    const scada::SessionDebugger::RequestEvent& event) {
  request.phase = event.phase;

  if (!event.title.empty()) {
    request.title = event.title;
  }

  if (!event.body.empty()) {
    request.body = event.body;
  }

  if (!event.response_body.empty()) {
    request.response_body = event.response_body;
  }
}
