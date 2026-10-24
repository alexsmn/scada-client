#pragma once

#include "print/service/print_util.h"
#include "scada/date_time.h"
#include "scada/date_time_range.h"

#include <optional>
#include <string>

// Builds the header a printout opens with: the view's title, then the period
// it covers (when the view has one), what the view says about its data (a
// summary's method and interval), and when it was printed (backlog 875).
PrintHeader MakePrintHeader(std::u16string title,
                            const std::optional<scada::TimeRange>& period,
                            std::u16string description,
                            scada::Time printed_at);
