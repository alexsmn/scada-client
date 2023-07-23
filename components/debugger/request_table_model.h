#pragma once

#include "aui/models/table_model.h"
#include "scada/session_debugger.h"

namespace scada {
class SessionService;
}

class RequestTableModel : public aui::TableModel {
 public:
  explicit RequestTableModel(scada::SessionService& session_service);

  using RequestId = scada::SessionDebugger::RequestId;
  using RequestPhase = scada::SessionDebugger::RequestPhase;

  struct Request {
    RequestId id = 0;
    RequestPhase phase = RequestPhase::Running;
    std::string title;
    std::string body;
    std::string response_body;
  };

  const Request& request(int index) const { return requests_[index]; }

  // aui::TableModel
  virtual int GetRowCount() override;
  virtual void GetCell(aui::TableCell& cell) override;

 private:
  void UpdateRequest(const scada::SessionDebugger::RequestEvent& event);

  std::vector<Request> requests_;

  std::unordered_map<RequestId, int /*index*/> request_id_to_index_;

  boost::signals2::scoped_connection request_event_connection_;
};
