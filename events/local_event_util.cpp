#include "events/local_event_util.h"

#include "base/strings/stringprintf.h"
#include "events/local_events.h"
#include "profile/profile.h"
#include "scada/localized_text.h"
#include "scada/status.h"

void ReportRequestResult(const std::u16string& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         const Profile& profile) {
  if (status && !profile.show_write_ok) {
    return;
  }

  scada::LocalizedText message = base::StringPrintf(
      u"%ls - %ls.", title.c_str(), ToString16(status).c_str());

  LocalEvents::Severity severity =
      status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;

  local_events.ReportEvent(severity, message);
}
