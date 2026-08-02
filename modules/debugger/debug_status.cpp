#include "modules/debugger/debug_status.h"

#include "base/utf_convert.h"

#include <algorithm>
#include <string>

namespace {

// ASCII case-fold (request titles are protocol/method names). Non-ASCII code
// units pass through unchanged.
std::u16string ToLowerAscii(std::u16string_view text) {
  std::u16string result{text};
  for (char16_t& c : result) {
    if (c >= u'A' && c <= u'Z')
      c = static_cast<char16_t>(c - u'A' + u'a');
  }
  return result;
}

}  // namespace

DebugStatus DebugStatusFor(scada::SessionDebugger::RequestPhase phase) {
  switch (phase) {
    case scada::SessionDebugger::RequestPhase::Succeeded:
      return DebugStatus::kOk;
    case scada::SessionDebugger::RequestPhase::Failed:
      return DebugStatus::kError;
    case scada::SessionDebugger::RequestPhase::Running:
      break;
  }
  return DebugStatus::kRunning;
}

bool DebugRequestMatches(const RequestTableModel::Request& request,
                         const std::u16string& query) {
  if (query.empty())
    return true;

  const std::u16string needle = ToLowerAscii(query);

  // Title substring (case-insensitive).
  if (ToLowerAscii(UtfConvert<char16_t>(request.title)).find(needle) !=
      std::u16string::npos) {
    return true;
  }

  // Request-id substring (so "402" finds request 4021).
  const std::u16string id = UtfConvert<char16_t>(std::to_string(request.request_id));
  return id.find(needle) != std::u16string::npos;
}
