#pragma once

#include <string>

namespace scada {
class Status;
}

class LocalEvents;
class Profile;

void ReportRequestResult(const std::u16string& title,
                         const scada::Status& status,
                         LocalEvents& local_events,
                         const Profile& profile);
