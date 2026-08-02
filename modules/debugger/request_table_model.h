#pragma once

#include "aui/models/table_model.h"
#include "base/lifetime.h"
#include "scada/session_debugger.h"

#include <chrono>

namespace scada {
class SessionService;
}

class RequestTableModel : public scada::aui::TableModel {
 public:
  explicit RequestTableModel(scada::SessionService& session_service);

  using RequestId = scada::SessionDebugger::RequestId;
  using RequestPhase = scada::SessionDebugger::RequestPhase;
  using RequestTime = std::chrono::system_clock::time_point;

  struct Request {
    RequestId request_id = 0;
    RequestPhase phase = RequestPhase::Running;
    RequestTime start_time;
    // Zero if the request is still running.
    RequestTime finish_time;
    std::string title;
    std::string body;
    std::string response_body;
  };

  // The request shown at visible row `index` (after the filter).
  const Request& request(int index) const SCADA_LIFETIME_BOUND {
    return requests_[visible_[index]];
  }

  // Filters the visible rows to requests matching `query` (title / id substring,
  // case-insensitive); empty shows all. Backs the trace-filter field.
  void SetFilter(std::u16string query);
  // Drops every captured request.
  void Clear();
  // Pauses / resumes capture: while paused, request events are ignored.
  void SetPaused(bool paused);
  bool paused() const { return paused_; }

  // aui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(scada::aui::TableCell& cell) override;

 private:
  void RebuildVisible();

  void ProcessRequestEvent(const scada::SessionDebugger::RequestEvent& event);

  // Returns false if the request is not found.
  bool UpdateRunningRequest(const scada::SessionDebugger::RequestEvent& event);

  void AddRequest(const scada::SessionDebugger::RequestEvent& event);
  void RemoveOldRequests();

  static bool IsRunningRequest(RequestPhase phase);

  static void UpdateRequestFromEvent(
      Request& request,
      const scada::SessionDebugger::RequestEvent& event);

  std::vector<Request> requests_;
  // Indices into requests_ that pass the current filter, in order.
  std::vector<int> visible_;
  std::u16string filter_;
  bool paused_ = false;

  std::unordered_map<RequestId, int /*index*/> running_request_id_to_index_;

  boost::signals2::scoped_connection request_event_connection_;
};

std::string DumpRequest(const RequestTableModel::Request& request);
