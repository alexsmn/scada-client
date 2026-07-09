#pragma once

#include "base/any_executor.h"

#include "base/awaitable.h"
#include "scada/data_services.h"

#include <functional>
#include <optional>

namespace Wt {
class WWidget;
}

struct DataServicesContext;

// Coroutine: `services_context` is taken by value (CP.53) — a reference
// parameter would dangle if the caller's frame dies before first resume.
Awaitable<std::optional<DataServices>> ExecuteLoginDialog(
    AnyExecutor executor,
    Wt::WWidget& parent,
    DataServicesContext services_context);
