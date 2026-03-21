#include "events/local_event_util.h"

#include "base/u16format.h"
#include "events/local_events.h"
#include "profile/profile.h"
#include "scada/localized_text.h"
#include "scada/status.h"

// Windows.h #defines ReportEvent to ReportEventA/W. Undo it.
#ifdef ReportEvent
#undef ReportEvent
#endif

void ReportRequestResult(const std::u16string& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         const Profile& profile) {
  if (status && !profile.show_write_ok) {
    return;
  }

  scada::LocalizedText message =
      u16format(L"{} - {}.", title, ToString16(status));

  LocalEvents::Severity severity =
      status ? LocalEvents::SEV_INFO : LocalEvents::SEV_ERROR;

  local_events.ReportEvent(severity, message);
}
