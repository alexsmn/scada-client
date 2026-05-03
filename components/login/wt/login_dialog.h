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

Awaitable<std::optional<DataServices>> ExecuteLoginDialog(
    AnyExecutor executor,
    Wt::WWidget& parent,
    DataServicesContext&& services_context);
