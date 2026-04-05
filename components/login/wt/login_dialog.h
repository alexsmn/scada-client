#pragma once

#include "base/promise.h"
#include "scada/data_services.h"

#include <functional>
#include <optional>

namespace Wt {
class WWidget;
}

class Executor;
struct DataServicesContext;

promise<std::optional<DataServices>> ExecuteLoginDialog(
    std::shared_ptr<Executor> executor,
    Wt::WWidget& parent,
    DataServicesContext&& services_context);
