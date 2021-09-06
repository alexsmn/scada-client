#pragma once

#include "base/promise.h"
#include "core/data_services.h"

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
